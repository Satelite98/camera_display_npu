[toc]

# linux 驱动学习

##　一、字符驱动

其次本次研究的为什么叫字符设备，就是因为在数据读取的时候都是`一个一个字节`的方式来传输的。

linux 中驱动的调用接口都如下图所示：任何的驱动文件成功加载之后都会在`/dev`目录下生成一个对应的文件。

![image-20241016213210180](D:\ForStudy\camera_dispaly_npu\camera_display_npu\notes\linux_driver\Untitled.assets\image-20241016213210180.png)

### 1.1 字符设备开发顺序

* **加载和卸载：**一个模块的驱动主要加载和卸载的接口就是`module_init(xxx_init);   `和`module_exit(xxx_exit); `两个函数。在实际的加载和卸载的指令的时候用的是`insmode drv.ko`和`rmmode drv.ko`这两个命令。

* **字符设备的注册和注销**：模块驱动的加载和卸载只是让内核代码中有这些模块的驱动？但是实际使用中我们还会为对于的设备分配设备号，将这些设备注册为字符设备。并且将设备的名字、主设备号、和操作函数联系在一起。`那么这里有个问题，如果是多个设备怎么处理的？`**--------等待回答**

  RE: 这里是主设备号，多个设备可能就是从设备号了。同时要注意新驱动选择没有被使用过的主设备号，利用`shell:cat /proc/devices  `可以看到已经被使用的主设备号。

  ```c
  static inline int register_chrdev(unsigned int major, const char *name, const struct file_operations *fops) 
  static inline void unregister_chrdev(unsigned int major, const char *name)
  ```

  一般情况下，我们会在驱动的加载和卸载的入口函数中将字符设备注册和注销

* **实现具体的设备操作函数：**，在这里就是实现字符设备操作节结构体`static struct file_operations`里面各个函数指针的实例化。

* **添加LICENSE和作者信息：**利用`MODULE_LICENSE("GPL");`和`MODULE_AUTHOR("zuozhongkai"); `来添加LICENSE和作者信息。

* **Example:**

  ```c
  29 static struct file_operations test_fops = { 
  30    .owner = THIS_MODULE,    
  31    .open = chrtest_open, 
  32    .read = chrtest_read, 
  33    .write = chrtest_write, 
  34    .release = chrtest_release, 
  35 }; 
  36  
  37 /* 驱动入口函数 */ 
  38 static int __init xxx_init(void) 
  39 { 
  40    /* 入口函数具体内容 */ 
  41    int retvalue = 0; 
  42  
  43    /* 注册字符设备驱动 */ 
  44    retvalue = register_chrdev(200, "chrtest", &test_fops); 
  45    if(retvalue < 0){ 
  46        /* 字符设备注册失败,自行处理 */ 
  47    } 
  48    return 0; 
  49 } 
  50  
  51 /* 驱动出口函数 */ 
  52 static void __exit xxx_exit(void) 
  53 { 
  54    /* 注销字符设备驱动 */ 
  55    unregister_chrdev(200, "chrtest"); 
  56 } 
  57  
  58 /* 将上面两个函数指定为驱动的入口和出口函数 */ 
  59 module_init(xxx_init); 
  60 module_exit(xxx_exit);
  61
  62 MODULE_LICENSE("GPL"); 
  63 MODULE_AUTHOR("zuozhongkai"); 
  ```

  

### 1.2 设备号

  设备号的类型定义见下文，本质上是一个无符号32位数据。并且对于32位机器来说，这低20位表示次设备号，高12位表示主设备号。

```c
/*设备号定义*/
typedef __u32 __kernel_dev_t;
typedef __kernel_dev_t		dev_t;

/*设备号相关的宏*/
#define MINORBITS	20
#define MINORMASK	((1U << MINORBITS) - 1)

#define MAJOR(dev)	((unsigned int) ((dev) >> MINORBITS))
#define MINOR(dev)	((unsigned int) ((dev) & MINORMASK))
#define MKDEV(ma,mi)	(((ma) << MINORBITS) | (mi))
```

* **分配设备号：**次设备号起始地址，alloc_chrdev_region 可以申请一段连续的多个设备号，这些设备号的主设备号一样，但是次设备号不同，次设备号以 baseminor 为起始地址地址开始递增。一般 baseminor 为 0，也就是说次设备号从 0 开始。 

  ```c
  /*动态申请设备号*/
  int alloc_chrdev_region(dev_t *dev, unsigned baseminor, unsigned count, const char *name)
  /*释放设备号*/
  void unregister_chrdev_region(dev_t from, unsigned count) 
  
  ```

### 1.3 编写第一个字符程序

​		具体的程序可见代码 `/home/wxwang/imax6ull/opendv/linux_kernel/drivers/my_driver/virtual_cchrdevbase.c`，这里面需要注意的就是介绍了**printk和copytouser**的函数的使用。根据编译报错问题，注意点如下：

* `__exit`中有两个下划线
* 当你去拷贝这些函数指针的时候`int (*release) (struct inode *, struct file *);`拷贝过来之后还需要对这些指针实例化见`int chrdevbase_release(struct inode *inode, struct file *filep)`

### 1.4 编写第一个测试程序及编译

### 1.5 驱动文件的编译和加载

#### 1.5.1 程序编译

* **程序编译：**make file和注释可见下面的代码段。在linux 环境中执行`make -j4`就能够编译成功啦。

```makefile
KERNELDIR := /home/wxwang/imax6ull/opendv/linux_kernel  #获取源码地址，应为有依赖关系
CURRENT_PATH := $(shell pwd)  #获取当前驱动路径
obj-m := virtualChardevbase.o #需要编译的模块
 
build:   kernel_modules #build 的依赖语法
 
kernel_modules:
#   echo $(KERNELDIR);
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules  #实际的编译指令
clean: 
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean 
```

#### 1.5.2 程序加载：

* 开发板和电脑的文件传输：这里可能需要从boot 相关的代码开始看。

  1. 硬件环境准备：用网线把电脑和ethNet2 连接起来。

  2. 设置电脑和开发板的网络/网关/ip地址等

  3. 验证网络相关命令

     ```c
     /* ping  */
     /* dhcp */
     /* nfs  */
     /* tftp */
     
     ```



---

### 二、LED灯驱动实验

#### 2.1 IO相关的操作

* **ioremap 和 iounmap: **

#### 2.2 LED 驱动的编写

* 硬件环境：LED 由GPIO port1-IO03控制、mmu

* 软件环境：由于有mmu，linux软件中使用了VA和PA的转换，我们只能知道想要访问的物理寄存器地址，但是不知道虚拟地址是什么，所以需要用`IO_remap`函数来进行一次转换，见下图：

```c
/*
*phys_addr :物理起始地址
*size: 需要映射的大小
*mtype:ioremap 的类型
*__iomem:一个__iomem类型的指针，指向的是映射后虚拟空间的首地址。
*/

void __iomem *
__arm_ioremap(phys_addr_t phys_addr, size_t size, unsigned int mtype)

/*
*io_addr :获取的IO map的地址
*/    
void __iounmap(volatile void __iomem *io_addr)
    
/*
*对地址的读写接口
*/
extern u8		readb(const volatile void __iomem *addr);
extern u16		readw(const volatile void __iomem *addr);
extern u32		readl(const volatile void __iomem *addr);
extern u64		readq(const volatile void __iomem *addr);
extern void		writeb(u8 b, volatile void __iomem *addr);
extern void		writew(u16 b, volatile void __iomem *addr);
extern void		writel(u32 b, volatile void __iomem *addr);
extern void		writeq(u64 b, volatile void __iomem *addr);
```









