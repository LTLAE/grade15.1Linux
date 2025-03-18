GZHU grade 15 Linux / Unix experiments personal backup
====
This repository is created for personal backup.  
Highly NOT recommand using codes in this repo to submit homework.  

experiment 1  
---
1. 利用fork函数创建新进程，并根据fork函数的返回值，判断自己是处于父进程还是子进程中  
2. 在新创建的子进程中，使用exec类的函数启动另一程序的执行；分析多进程时系统的运行状态和输出结果  
3. 利用最常用的三个函数pthread_create，pthread_join和pthread_exit编写了一个最简单的多线程程序。理解多线程的运行和输出情况  
4. 利用信号量机制控制多线程的运行顺序，并实现多线程中数据的共享；分析Linux系统下多进程与多线程中的区别  
5. 编写程序实现进程的管道通信。用系统调用pipe( )建立一管道，二个子进程P1和P2分别向管道各写一句话：  
Child 1 is sending a message!  
Child 2 is sending a message!  
父进程从管道中读出二个来自子进程的信息并显示（要求先接收P1，后P2）  
6. 编写一个HelloWorld内核模块，并进行装载和卸载操作  
7. 用fork写一个测试程序，从父进程和子进程中打印出各自的PID，并把wait(),exit()系统调用加进去，使子进程返回退出状态给父进程，并将它包含在父进程的打印信息中。对运行结果进行分析，并通过dmesg命令查看系统创建进程过程  
8. 编写一个内核模块，用于打印进程描述符task_struct中的如下字段：当前系统中每个进程的进程标识号（p->pid）、进程状态（p->state）、进程的优先级（p->prio）、进程静态优先级（ p->static_prio）、父进程的进程号（ (p->parent)->pid）、进程权限位的默认设置(p->fs)->umask)，并打印出进程地址空间信息（(p->mm)->total_vm），统计当前系统的进程个数（count）

experiment 2  
---
1. 利用strace命令跟踪进程调用exec函数将可执行文件载入内存时，代码段，数据段，bss 段，stack 段通过 mmap 函数映射到内存空间的过程  
2. 利用free命令查看文件加载进内存前后系统内存的使用情况  
3. 利用链表结构，malloc函数和 free函数实现将终端输入的一系列字符串用链表的形式保存下来。然后再将这些数据组装起来，回显到输出终端  
4. 教材中的用户程序读取内核空间的内存管理实例（用户与内核共享内存实例）  
编写一个虚拟字符设备驱动模块，在其驱动中完成设备空间，也就是内核空间到用户空间的内存映射，即将内核空间的线性地址所对应的物理地址映射到用户空间的线性地址中，从而实现在用户空间直接读取内核数据的目的。使得访问用户空间地址等同于访问被映射的内核空间地址，从而不再需要数据拷贝操作。这样便可进行内核空间到用户空间的大规模信息传送，从而应用于高速数据采集等性能要求高的场合。

experiment 3  
---
1. 编写一个内核模块，在/proc文件系统中增加一个目录hello，并在这个目录中增加一个文件world，文件的内容为hello world
2. 编写一个有两个进程并发运行的程序，并在结尾设定一个死循环，编译完成后让其在后台运行，然后在前台用cat 命令查看/proc目录下的文件/proc/meminfo （当前内存信息）、/proc/vmstat （虚拟内存统计信息）、/proc/vmallocinfo（虚拟内存分配息）、/proc/filesystems（目前系统支持的文件系统）、/proc/locks（当前被内核锁定的文件）、/proc/cpuinfo （ cpu相关信息）和/proc/modules（当前系统已经加载的模块（lsmod））中信息，分析其含义

experiment 4  
---
1. 编写一个字符设备驱动程序，并在设备的打开操作中打印主次设备号
2. 编写一个块设备驱动程序，并实现设备的读写操作

course design  
---
为Linux操作系统设计一个简单文件系统  

