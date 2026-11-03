# Learn comprehensive linux with Beaglebone Black (hardware branch)

## Documents
- [The Linux Kernel Module Programming Guide (Peter Jay Salzman, Michael Burian, Ori Pomerantz, Bob Mottram, Jim Huang)](https://sysprog21.github.io/lkmpg/)
- Linux System Programming (Robert Love)
<!-- - [Linux Kernel Teaching](https://linux-kernel-labs.github.io/) -->
<!-- - Mastering Embedded Linux Development 4th (Frank Vasquez, Chris Simmonds) -->
<!-- - Linux Device Driver 3rd (Jonathan Corbet, Alessandro Rubini, Greg Kroah-Hartman) -->

## Project tree
### [Basice LKMPG](basic/README.md)
- [x] b0: [Simple driver](basic/b0/README.md)
- [x] b1: [A normal driver and some "shell" module that currently only for know how to use (including procfs, sysfs, debugfs, ioctl)](basic/b1/README.md)
- [x] b2: [Pseudo ioctl character device driver and its user space application](basic/b2/README.md)
- [ ] [wft] b3: [Syscall probe/ kernel module](basic/b3/README.md)

### LKMPG & Device driver
- [ ]  [wip] d0: GPIO
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