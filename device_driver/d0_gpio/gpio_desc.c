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

#include <asm/errno.h>

#define DEVICE_NUM              4
#define DEVICE_NAME_PREFIX      "user_leds_"
#define DEVICE_TREE_NAME_PREFIX "led"

static struct gpio_desc *led[DEVICE_NUM];

static int major = 0;
static int minor = 0;
static dev_t devno;
static struct cdev cdev;
static struct class *class;
static struct device *device[DEVICE_NUM];

static int dtb_used = 1;
module_param(dtb_used, int, 0644);

static struct gpiod_lookup_table leds_gpio_table_0 = {
    .list = LIST_HEAD_INIT(leds_gpio_table_0.list),
    .dev_id = "user_leds_0",
    .table = {
        GPIO_LOOKUP("gpio-0-31", 21, "user_led", GPIO_ACTIVE_HIGH),
        { }
    },
};

static struct gpiod_lookup_table leds_gpio_table_1 = {
    .list = LIST_HEAD_INIT(leds_gpio_table_0.list),
    .dev_id = "user_leds_1",
    .table = {
        GPIO_LOOKUP("gpio-0-31", 22, "user_led", GPIO_ACTIVE_HIGH),
        { }
    },
};

static struct gpiod_lookup_table leds_gpio_table_2 = {
    .list = LIST_HEAD_INIT(leds_gpio_table_0.list),
    .dev_id = "user_leds_2",
    .table = {
        GPIO_LOOKUP("gpio-0-31", 23, "user_led", GPIO_ACTIVE_HIGH),
        { }
    },
};

static struct gpiod_lookup_table leds_gpio_table_3 = {
    .list = LIST_HEAD_INIT(leds_gpio_table_0.list),
    .dev_id = "user_leds_3",
    .table = {
        GPIO_LOOKUP("gpio-0-31", 24, "user_led", GPIO_ACTIVE_HIGH),
        { }
    },
};

static struct gpiod_lookup_table *leds_gpio_table[DEVICE_NUM] = {
    &leds_gpio_table_0,
    &leds_gpio_table_1,
    &leds_gpio_table_2,
    &leds_gpio_table_3,
};

static int get_led_from_dt(int i)
{
    struct device_node *parent, *child;
    char name[8];

    parent = of_find_node_by_path("/leds");
    if (!parent)
        return -ENODEV;

    snprintf(name, sizeof(name), "led%d", i + 2);
    child = of_get_child_by_name(parent, name);
    of_node_put(parent);
    if (!child)
        return -ENODEV;

    led[i] = fwnode_gpiod_get_index(of_fwnode_handle(child), NULL, 0,
                                    GPIOD_OUT_LOW, "user_led");
    of_node_put(child);

    return IS_ERR(led[i]) ? PTR_ERR(led[i]) : 0;
}

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

    if (length < 1) {
        return -EINVAL;
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

static int __init device_init(void)
{
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

    int i = 0;
    for (i = 0; i < DEVICE_NUM; ++ i) {
        char device_name[12];
        if (dtb_used) {
            sprintf(device_name, "%s%d", DEVICE_TREE_NAME_PREFIX, i + 2);
        }
        else {
            sprintf(device_name, "%s%d", DEVICE_NAME_PREFIX, i);
        }
        device[i] = device_create(class, NULL, MKDEV(major, minor + i), NULL, device_name);
        if (IS_ERR(device[i])) {
            retval = PTR_ERR(device[i]);
            pr_err("device_init: device_create %d failed %d\n", i, retval);
            goto err_device_destroy;
        }
    }

    if (dtb_used) {
        pr_info("device_init: device created at /dev/%s*\n", DEVICE_TREE_NAME_PREFIX);
    }
    else {
        pr_info("device_init: device created at /dev/%s*\n", DEVICE_NAME_PREFIX);
    }
    for (i = 0; i < DEVICE_NUM; ++i) {
        if (dtb_used)
            retval = get_led_from_dt(i);
        else {
            gpiod_add_lookup_table(leds_gpio_table[i]);
            led[i] = gpiod_get(device[i], "user_led", GPIOD_OUT_LOW);
            retval = IS_ERR(led[i]) ? PTR_ERR(led[i]) : 0;
        }
        if (retval) {
            pr_err("device_init: gpiod_get %d failed %d\n", i, retval);
            goto err_gpio_put;
        }
    }
    
    return 0;

err_gpio_put:
    for (--i ; i >= 0; -- i) {
	    gpiod_put(led[i]);
        if (!dtb_used) {
            gpiod_remove_lookup_table(leds_gpio_table[i]);
        }
    }
    i = DEVICE_NUM; 
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

static void __exit device_exit(void)
{
    int i;

    if (!dtb_used) {
        for (i = 0; i < DEVICE_NUM; ++i) {
            gpiod_remove_lookup_table(leds_gpio_table[i]);
        }
    }

    for (i = 0; i < DEVICE_NUM; ++ i) {
        gpiod_set_value(led[i], 0);
        gpiod_put(led[i]);
        device_destroy(class, devno + i);
    }
    class_destroy(class);
    cdev_del(&cdev);
    unregister_chrdev_region(devno, DEVICE_NUM);

    pr_info("device_exit: device removed\n");
}

module_init(device_init);
module_exit(device_exit);

MODULE_AUTHOR("friendlydenji");
MODULE_DESCRIPTION("gpio device driver");
MODULE_LICENSE("GPL");