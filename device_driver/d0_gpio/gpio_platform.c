#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/printk.h>
#include <linux/uaccess.h>
#include <linux/moduleparam.h>
#include <linux/gpio/machine.h>
#include <linux/of.h>
#include <linux/gpio/consumer.h>
#include <linux/platform_device.h>  
#include <linux/property.h>    

#include <asm/errno.h>

#define DEVICE_NUM              4
#define DEVICE_NAME_PREFIX      "user_leds_"

static int major = 0;
static int minor = 0;
static dev_t devno;
static struct cdev cdev;
static struct class *class;
static struct device *device[DEVICE_NUM];
static struct gpio_desc *led[DEVICE_NUM];

enum led_dt_mode {
    LED_FROM_PROP = 1,    
    LED_FROM_CHILD,       
};

static int device_open(struct inode *inode, struct file *filp) 
{
    int led_index = iminor(inode) - minor;   

    if (led_index < 0 || led_index >= DEVICE_NUM)
        return -ENODEV;

    filp->private_data = (void *)(uintptr_t)led_index;  
    return 0;
}

static int device_release(struct inode *inode, struct file *filp) 
{
    return 0;
}

static ssize_t device_read(struct file *filp, char __user *buffer, size_t length, loff_t *offset)
{
    int led_index = (uintptr_t)filp->private_data;
    char value;

    if (length == 0 || *offset > 0) {
        return 0;     
    }                 

    value = '0' + gpiod_get_value(led[led_index]);
    if (copy_to_user(buffer, &value, 1)) {
        return -EFAULT;
    }
        
    *offset += 1;
    return 1;
}

static ssize_t device_write(struct file *filp, const char __user *buffer, size_t length, loff_t *offset)
{
    int led_index = (uintptr_t)filp->private_data;

    char value;

    if (length == 0) {
        return 0;
    }

    if (copy_from_user(&value, buffer, 1))
        return -EFAULT;

    if (value != '0' && value != '1')
        return -EINVAL;

    gpiod_set_value(led[led_index], value == '1');

    return length;
}

static struct file_operations fops = {
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 10, 0)
    .owner = THIS_MODULE,
#endif
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .release = device_release
};

static int led_probe(struct platform_device *pdev)
{
    enum led_dt_mode mode = (enum led_dt_mode)(uintptr_t)device_get_match_data(&pdev->dev);

    int i = 0;
    switch (mode) {
        case LED_FROM_PROP:
            for (; i < DEVICE_NUM; ++ i) {
                    led[i] = devm_gpiod_get_index(&pdev->dev, "led", i, GPIOD_OUT_LOW);   
                    if (IS_ERR(led[i])) {
                        return dev_err_probe(&pdev->dev, PTR_ERR(led[i]), "cannot get led-gpios %d\n", i);
                    }
            }
            break;
        case LED_FROM_CHILD: {
            struct fwnode_handle *child;
            device_for_each_child_node(&pdev->dev, child) {
                if (i >= DEVICE_NUM) {
                    fwnode_handle_put(child);
                    break;
                }
                led[i] = devm_fwnode_gpiod_get(&pdev->dev, child, NULL, GPIOD_OUT_LOW, "user_led");
                if (IS_ERR(led[i])) {
                    fwnode_handle_put(child);
                    return dev_err_probe(&pdev->dev, PTR_ERR(led[i]), "cannot get gpios of child %d\n", i);
                }
                i ++;
            }
            break;
        }
        default:
            pr_err("invalid\n");
            return -ENODEV;
            break;
    }
    if (i != DEVICE_NUM) {
        return dev_err_probe(&pdev->dev, -EINVAL, "expected %d led child nodes, found %d\n", DEVICE_NUM, i);
    }

    int retval = 0;

    if (major) {
        devno = MKDEV(major, minor);
        retval = register_chrdev_region(devno, DEVICE_NUM, DEVICE_NAME_PREFIX);
        if (retval) {
            pr_err("device_init: register_chrdev_region failed %d\n", retval);
            goto err_out;
        }
    }
    else {
        retval = alloc_chrdev_region(&devno, minor, DEVICE_NUM, DEVICE_NAME_PREFIX);
        if (retval) {
            pr_err("device_init: alloc_chrdev_region failed %d\n", retval);
            goto err_out;
        }
        major = MAJOR(devno);
    }
    pr_info("device_init: device registered with major %d, minor %d\n", major, minor);

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 4, 0)
    cdev.owner = THIS_MODULE;
#endif

    cdev_init(&cdev, &fops);
    retval = cdev_add(&cdev, devno, DEVICE_NUM);
    if (retval) {
        pr_err("device_init: cdev_add failed %d\n", retval);
        goto err_unregister_chardev_region;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
    class = class_create(DEVICE_NAME_PREFIX);
#else
    class = class_create(THIS_MODULE, DEVICE_NAME_PREFIX);
#endif
    if (IS_ERR(class)) {
        retval = PTR_ERR(class);
        pr_err("device_init: class_create failed %d\n", retval);
        goto err_cdev_del;
    }

    for (i = 0; i < DEVICE_NUM; ++ i) {
        device[i] = device_create(class, &pdev->dev, devno + i, NULL, DEVICE_NAME_PREFIX "%d", i);
        if (IS_ERR(device[i])) {
            retval = PTR_ERR(device[i]);
            pr_err("device_init: device_create %d failed %d\n", i, retval);
            goto err_device_destroy;
        }
    }

	dev_info(&pdev->dev, "LED device probed\n");
	return 0;

err_device_destroy:
    for (-- i; i >=0 ; -- i) {
        device_destroy(class, devno + i);
    }
    class_destroy(class);
err_cdev_del:
    cdev_del(&cdev);
err_unregister_chardev_region:
    unregister_chrdev_region(devno, DEVICE_NUM);
err_out:
    return retval;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 12, 0)
static void led_remove(struct platform_device *pdev)
#else
static int led_remove(struct platform_device *pdev)
#endif
{
    int i;

    for (i = 0; i < DEVICE_NUM; ++i) {
        gpiod_set_value(led[i], 0); 
        device_destroy(class, devno + i);
    }

    class_destroy(class);
    cdev_del(&cdev);
    unregister_chrdev_region(devno, DEVICE_NUM);
    major = 0;

    dev_info(&pdev->dev, "LED device removed\n");
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 12, 0)
    return 0;
#endif
}

static const struct of_device_id leds_of_match[] = {
    { .compatible = "user-leds", .data = (void *)LED_FROM_PROP },
    { .compatible = "gpio-leds", .data = (void *)LED_FROM_CHILD },
    { }
};
MODULE_DEVICE_TABLE(of, leds_of_match);

static struct platform_driver led_driver = {
	.probe = led_probe,
    .remove = led_remove,
	.driver = {
		.name = "user_leds",
		.of_match_table = leds_of_match,
	},
};
module_platform_driver(led_driver);

MODULE_AUTHOR("friendlydenji");
MODULE_DESCRIPTION("gpio platform device driver");
MODULE_LICENSE("GPL");