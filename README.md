# Learn comprehensive linux with Beaglebone Black
This is the nth time I've restarted the Beaglebone Black embedded Linux study and this time maybe I'll make it a comprehensive course to learn Linux.

## Prerequisites
- Before diving into this shjt, we could download all stuffs first to avoid wasting time to wait them in the future.
- I divide them into 2 groups of device: BBB and QEMU on x86 host
- And 3 groups of hardware (BBB) images: BeagleBoard images, build system images and scratch linux system.
- Every group has its own advantages and goals to achieve.

### QEMU
- Fast deploy and convenient for Linux kernel module and Linux system programming

### Image provided by BeagleBoard 
- Fast deploy and convenient for Linux kernel module and Linux system programming 
- Available from https://www.beagleboard.org/distros

### Build system images
- Including Yocto and Buildroot. 
- Buildroot help us reduce build time and easier when compare to Yocto, but Yocto give us better performance.

### Embedded Linux from "scratch"
- Not the Linux From Scrath (If I have a lot of time, maybe I'll try).
- Build components of a embedded Linux system (toolchains, bootloader, kernel, rootfs) separate then gather them to boot the board.
- This method help us understand more about the boot flow and has high customization.

## My plan
- Currently, we will focus on 3 aspects of Linux programming including: Linux kernel module, Linux system programming and device driver.
- For Linux kernel module and Linux system programming, we can both test them on Beaglebone Black or QEMU.
- For device driver, we will test on the real hardware bbb for sure.

## Documents
- [The Linux Kernel Module Programming Guide (Peter Jay Salzman, Michael Burian, Ori Pomerantz, Bob Mottram, Jim Huang
)](https://sysprog21.github.io/lkmpg/)
- Linux System Programming (Robert Love)
<!-- - [Linux Kernel Teaching](https://linux-kernel-labs.github.io/) -->
<!-- - Mastering Embedded Linux Development 4th (Frank Vasquez, Chris Simmonds) -->
<!-- - Linux Device Driver 3rd (Jonathan Corbet, Alessandro Rubini, Greg Kroah-Hartman) -->

## Progress
- At this time, we have 3 main branches that ***main*** (this branch) for introducing, ***bbb*** for test on the hardware and ***qemux86*** for test on qemu.
- Some random module will have debug version for debugging.

### Basic LKMPG
- [x] b0: Simple driver 
- [x] b1: A normal driver and some "shell" module that currently only for know how to use (including procfs, sysfs, debugfs, ioctl) 
- [x] b2: Pseudo ioctl character device driver and its user space application 
- [ ] b3: Syscall probe/ kernel module 

### LKMPG & Device driver
- [ ] d0: GPIO
- [ ] d1: Sensor
- [ ] d2: USB driver
- [ ] d3: PCI Ethernet driver
- [ ] d4: USB Ethernet driver
- [ ] d5: Block device

### LKMPG & System programming
- [ ] s0: Buffered I/O 
- [ ] s1: Multiplexed I/O 
- [ ] s2: Asynchronous I/O 
- [ ] s3: Scatter/gather I/O 
- [ ] s4: Advise mechanism
- [ ] s5: Daemon 
- [ ] s6: Scheduling 
- [ ] s7: Cooperation 
- [ ] s8: Real-time 
- [ ] s9: Coroutine 

