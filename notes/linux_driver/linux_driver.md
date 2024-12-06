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

     * **WSL端**

     ```c
     /* 安装服务 */
     sudo apt-get install nfs-kernel-server rpcbind 
     /* 创建文件夹 &更改文件  */
     mkdir nfs & pwd
     sudo vi /etc/exports ->
     
     /* 文件中写入 */   
     /home/wxwang/imax6ull/opendv/nfs *(rw,sync,no_root_squash,no_subtree_check) 
         
     /*重启服务  */
     sudo /etc/init.d/nfs-kernel-server restart
         
         
      
      /* 实际操作 */
      sudo systemctl start rpcbind
 		sudo /etc/init.d/nfs-kernel-server restart
         
     ```
     
     

     * 参考链接:https://github.com/microsoft/WSL/issues/6113 
     * https://www.cnblogs.com/weigege2015/p/7398269.html
     
     * **board 端：这里实际介绍的是boot 里面的NFS**
     
     ```c
          /*1.利用boot设置IP地址*/
          setenv ipaddr 192.168.1.50 
          setenv ethaddr b8:ae:1d:01:00:00 
          setenv gatewayip 192.168.1.1 
          setenv netmask 255.255.255.0 
          setenv serverip 192.168.1.253 
          saveenv 
              
          /*2.pin 服务器  */
          ** 问题: 怎么确认WSL虚拟机的IP地址？ **
          
          /*3.启用NFS */
          nfs [loadAddress] [[hostIPaddr:]bootfilename] 
          /* 192.168.1.253是服务器地址  */
          nfs 80800000 192.168.1.253:/home/zuozhongkai/linu/nfs/zImage 
     ```



* 实际的配置可以borad 端的作用可以参考这个：https://blog.csdn.net/Wu_GuiMing/article/details/115859657
  
* 参考这个视频：网络环境搭建 https://www.yuanzige.com/course/detail/50096



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

由以上的接口，就可以进行LED灯的控制了，可以确认下面这些事情：

1. 控制LED灯的寄存器需要哪些，如何进行映射。LED init /LED ON/LED OFF的逻辑如何控制？

   **RE:**本质上就是控制GPIO，按照imx6ull的官方手册，可以分为下面几步：

   ![image-20241028175456471](.\linux_driver.assets\image-20241028175456471.png)

2. 再linux 中，利用`io_map`的特性去实现上述逻辑，并且将控制加入字符框架。



----

### 三、新字符设备驱动实验

#### 3.1 建议使用新的设备号注册和注销接口

* 设备号注册：`int alloc_chrdev_region(dev_t *dev, unsigned baseminor, unsigned count, const char *name) `
* 设备号注销：`void unregister_chrdev_region(dev_t from, unsigned count) `

#### 3.2 接入Cdev的字符设备框架

* Cdev 节点

```c
struct cdev {
	struct kobject kobj;
	struct module *owner;
	const struct file_operations *ops;/*操作符*/
	struct list_head list;
	dev_t dev;        /*设备号*/
	unsigned int count;
};

/* 定义节点 */
struct cdev test_cdev; 
```

* **1.定义节点**

  ```c
  struct cdev test_cdev; 
  ```

* **2.节点初始化：**和操作符匹配

  ```c
  1  struct cdev testcdev; 
  2   
  3  /* 设备操作函数 */ 
  4  static struct file_operations test_fops = { 
  5     .owner = THIS_MODULE, 
  6     /* 其他具体的初始项 */ 
  7  }; 
  8   
  9  testcdev.owner = THIS_MODULE; 
  10 cdev_init(&testcdev, &test_fops); /* 初始化 cdev 结构体变量 */ 
  ```

* **3.添加字符设备到cdev系统里**

  ```c
  int cdev_add(struct cdev *p, dev_t dev, unsigned count) 
  ```

* **4.从cdev系统中删除设备**

  ```c
   cdev_del(&testcdev)
  ```

#### 3.3 自动创建设备节点

​		需要根文件系统支持，利用的是`busybox`的`mdev`调用，需要在`/etc/init.d/rcS`中加入下列语句：

```c
echo /sbin/mdev > /proc/sys/kernel/hotplug 
```

* **创建设备类：**在Linux中，设备的管理是利用类来进行管理的，可以用class的方式将功能相似的设备归为一类。此外，需要需要**自动**在`/dev`下生成对应的设备节点，只有利用创建class之后利用`device_create`的方式可以自动创建。

  ```c
  /* 创建类 */
  struct class *class_create (struct module *owner, const char *name) 
  
  /* 删除类 */
  void class_destroy(struct class *cls);    
  ```

* **创建设备节点：**

  调用下面的接口创建设备类

  ```c
  struct device *device_create(struct class *class,  /* 类名 */
                               struct device *parent, /*父设备 */
                               dev_t devt,  /* 设备号 */
                               void *drvdata, /* 设备可能使用的数据 */
                               const char *fmt, ...)/*设备名字*/
  ```

  删除设备类：`device_destroy(newchrled.class, newchrled.devid);`

* 把一个设备用到的变量都抽象，并且给到设备文件中。

  ==问题：为什么需要给到设备文件的private_data 参数？==

  **RE:** 相当于有个指针指向了这个dev ,后续再用这个设备的时候，就能够用指针的方式，直接访问数据了。其中`struct file `是一个很大的结构体，会涉及到文件系统

  ```c
  /* 设备结构体 */ 
  1  struct test_dev{ 
  2     dev_t devid;              /* 设备号      */ 
  3     struct cdev cdev;         /* cdev       */ 
  4     struct class *class;     /* 类         */ 
  5     struct device *device;   /* 设备       */ 
  6     int major;                /* 主设备号     */ 
  7     int minor;                /* 次设备号     */ 
  8  }; 
  
  10 struct test_dev testdev; 
  
  13 static int test_open(struct inode *inode, struct file *filp) 
  14 { 
  15    filp->private_data = &testdev; /* 设置私有数据 */ 
  16    return 0; 
  17 } 
  ```

----

### 四、设备树

​	设备树（dts）是一种device信息，即硬件板子信息的表达方式。从uboot到Linux kernel的跳转过程可知，最终的我们用到的文件是dtb文件。从dts到dtb需要用到linux kernel 自带的dtc工具。利用`make all`或者`make dtbs`能够编译出dts文件。

​	如何确定编译的是哪个dts?在` arch/arm/boot/dts/Makefile`中寻找你需要的dts是被哪个编译选项控制的，例如imx6ull相关的dts就被宏` dtb-$(CONFIG_SOC_IMX6ULL) += `控制编译。

#### 4.1 DTS语法

​	dts是用来描述板子的硬件信息的，学习如何表达硬件信息是和语法强相关的。对于硬件信息来说，需要描述的有**IO复用、中断、寄存器、外设IP、CPU**这些信息，接下来我们看看是如何表达的。

* **设备树之间可以相互引用：**设备树之间可以像C语言那样相互引用，并且设备树中可以引用`dtsi &　dts`文件。如下所示，一般情况下，dtsi文件是用来描述SOC core及外设IP的信息。

  ```c
  #include <dt-bindings/input/input.h>
  #include "imx6ull.dtsi"
  
  ......
  ```

* **设备节点：**设备树描述信息的时候，如同一个树状分支一样，对于硬件IP信息做全面的说明

  * 根节点：dts用`/`作为根节点的起始，每个设备树文件只有一个根节点。当你的设备树文件包含其他设备树文件时，两个根节点会合并成一个根节点。

  * 子节点：命名规则为`label:node-name@unit-address`其中`node-name`是子节点的名字，`unit-address`是子节点的**设备地址（总线从设备）**或者是寄存器首地址。`label`是子节点的符号，目的是为了访问子节点并且添加内容，当没有符号时可以忽略。

    ```c
    /* 根节点 */
    /{
        /*子节点属性*/
        intc: interrupt-controller@00a01000{
            ......
                ......
        }
    
        &intc{
            ......
                ......
        }
    
    }
    ```


* **特殊节点：**设备树中有两个特殊的节点，**aliases和chosen**,其中aliases节点一般是作为别名节点，里面利用符号的方式，定义了很多符号的别名，便于引用。chosen 节点并不是一个设备节点，其作用在**于uboot向Linux内核传递数据，重点是bootargs**。uboot会dtb 文件中的chonsen 信息下面加入bootargs的信息。于是可以在编译之后的linux 的 `/proc/device-tree/base/chosen/`目录下看到bootargs的值

  ```c
  /* aliases node */
  18 aliases { 
  19    can0 = &flexcan1; 
  20    can1 = &flexcan2; 
  21    ethernet0 = &fec1; 
  22    ethernet1 = &fec2; 
  23    gpio0 = &gpio1; 
  24   ...
  25   ...	
  26  };
  
  /* chosen node */  
  18 chosen { 
  19    stdout-path = &uart1; 
  20 }; 
  ```

  

* **节点属性：**上面说明了设备树由根节点和子节点组成，那么如何对一个节点进行描述呢？我们一般会有以下这些属性来描述一个节点的硬件内容：

  * **compatible属性：**描述该节点的名字信息，是和驱动中的`xxxx_of_match`的设备ID表相匹配，该ID表最后会被赋值到`driver 层的of_match_table`，匹配后会执行`driver `的probe和remove函数。**每个节点都有compatible属性,包括根节点**

    ```c
    static const struct of_device_id xxxx_of_match[] = {
    	{ .compatible = "gpio-keys", },
    	{ },
    };
    
    static struct platform_driver gpio_keys_device_driver = {
    	.probe		= gpio_keys_probe,
    	.remove		= gpio_keys_remove,
    	.driver		= {
    		.name	= "gpio-keys",
    		.pm	= &gpio_keys_pm_ops,
    		.of_match_table = of_match_ptr(gpio_keys_of_match), /* of match table的设定 */
    	}
    };
    ```

  * **model属性：**字符串描述，无实际意义，可以用来描述模块信息。

  * **status属性：**用来描述描述设备状态，一般有`okey、disabled、fail、fail-sss`可选

  * **#address-cells 和#size-cells属性：**这两个属性描述了该节点中<reg>的格式。

    一般情况下reg的格式为：`reg = <address1 legth1 address2 length2....  >`这里面address描述起始地址，legth描述寄存器长度。而#address-cells 和#size-cells就分别描述adress和legth 所占的地址长度。可见下方demo

    ```c
    	spi4 {
    		compatible = "spi-gpio";
    		status = "disabled";
    		#address-cells = <1>;
    		#size-cells = <0>;
    
    		gpio_spi: gpio_spi@0 {
    			compatible = "fairchild,74hc595";
    			reg = <0>;
    			registers-default = /bits/ 8 <0x57>;
    			spi-max-frequency = <100000>;
    		};
    	};
    ```

  * **reg属性：**一般由（address legth）构成，这里比较重要的一般是**address**，因为一般对于代码而言，获取寄存器首地址即可。

  * range属性：能够描述一个偏移的地址空间，一般由< **子节点偏移地址 父节点实际地址 地址长度 >**三个信息组成，此信息就是描述实际的子节点起始地址=父节点实际地址+子节点偏移地址+ reg属性地址，可见下面案例：

    ```c
    1  soc { 
    2     compatible = "simple-bus"; 
    3     #address-cells = <1>; 
    4     #size-cells = <1>; 
    5     ranges = <0x0 0xe0000000 0x00100000>; 
    6    
    7      serial { 
    8          device_type = "serial"; 
    9          compatible = "ns16550"; 
    10        reg = <0x4600 0x100>;     /*子节点的实际起始地址为：0xe0000000+0x0+0x4600*/
    11        clock-frequency = <0>; 
    12        interrupts = <0xA 0x8>; 
    13        interrupt-parent = <&ipic>; 
    14    }; 
    15 a}; 
    ```

  * device_type属性：只有IEEE1275会用到该属性

#### 4.2 linux启动识别CPU的方法

##### 4.2.1没有设备树的情况

​	在没有设备树的情况下就是通过定义一个` .arch.info.init`的段的信息，然后通过链接文件链接到固定的地址，之后在从该地址遍历寻找，该地址的info信息是否匹配，最后boot的一个过程。

##### 4.2.1利用了设备树的情况

​	在linux中有`DT_MACHINE_START`的定义，同时在目标的CPU中有具体详细的定义。

```c
#define DT_MACHINE_START(_name, _namestr)		\
static const struct machine_desc __mach_desc_##_name	\
 __used							\
 __attribute__((__section__(".arch.info.init"))) = {	\
	.nr		= ~0,				\
	.name		= _namestr,

#endif


static const char *imx6ul_dt_compat[] __initconst = {
	"fsl,imx6ul",
	"fsl,imx6ull",
	NULL,
};

/* 具体的实际实现 */
DT_MACHINE_START(IMX6UL, "Freescale i.MX6 Ultralite (Device Tree)")
	.map_io		= imx6ul_map_io,
	.init_irq	= imx6ul_init_irq,
	.init_machine	= imx6ul_init_machine,
	.init_late	= imx6ul_init_late,
	.dt_compat	= imx6ul_dt_compat,
MACHINE_END
    
/*实际上就等于*/    
static const struct machine_desc __mach_desc_IMX6UL __used							\
 __attribute__((__section__(".arch.info.init"))) = {
    	.nr		= ~0,	
    	.name		= "Freescale i.MX6 Ultralite (Device Tree)"，
        .map_io		= imx6ul_map_io,
    	.init_irq	= imx6ul_init_irq,
    	.init_machine	= imx6ul_init_machine,
    	.init_late	= imx6ul_init_late,
    	.dt_compat	= imx6ul_dt_compat,
}
```

​	以上有一个**dt_compat**属性，在此C文件中的定义是`imx6ul_dt_compat`，其中列举了两个定义值，这两个字符变量，只要能够和设备树根节点的`compatible`属性匹配起来，就能够找到该设备树正常启动。其基本的匹配思路如下所示：

​	linux kernel 的入口函数为`/init/main.c`的`start_kernel`函数。后续的调用关系见下方所示：

* ```c
  start_kernel
      ->setup_arch
      	->setup_machine_fdt(__atags_pointer);/*参数是dtb的首地址*/
      		->of_flat_dt_match_machine(mdesc_best, arch_get_next_mach)/*这里的传参 一个mdesc_best是目标函数，arch_get_next_mach传入的函数指针*/
  
                  
  /* 函数具体实现 */                
  static const void * __init arch_get_next_mach(const char *const **match)
  {
  	static const struct machine_desc *mdesc = __arch_info_begin;
  	const struct machine_desc *m = mdesc;
  
  	if (m >= __arch_info_end)
  		return NULL;
  
  	mdesc++;
  	*match = m->dt_compat;
  	return m;
  }  
  
  /*函数的使用方法*/
  while ((data = get_next_compat(&compat))) {
  		score = of_flat_dt_match(dt_root, compat);
  		if (score > 0 && score < best_score) {
  			best_data = data;
  			best_score = score;
  		}
  	}
  ```

  由此可见，寻找设备树就是用函数传参**match**和该设备树的**m->dt_compat**中内容比较，如果找到匹配的话就会执行返回**machine_desc**这样的一个结构体(就是上面imx6ull定义的结构体)，同时这个值赋给`best_data`把找到的score的值赋给`best_score`开展后面的初始化。



#### 4.3 在设备树中增加/修改哪个文件

​			在设备树中，设备树dts文件也能引用其他的设备树，比如` imx6ull-alientek-emmc.dts`设备树文件就引用了` imx6ull.dtsi `文件。在` imx6ull.dtsi `文件中会描述这个SOC 的各个外设节点。如果我们有修改的需求，不能直接在` imx6ull.dtsi `中修改，因为不是所有该SOC的板级都有相同的外设，而是应该根据实际情况，利用**符号引用的方式**在`imx6ull-alientek-emmc.dts`的目录下修改。

```c
/* imx6ull.dtsi */
937 i2c1: i2c@021a0000 { 
938     #address-cells = <1>; 
939     #size-cells = <0>; 
940     compatible = "fsl,imx6ul-i2c", "fsl,imx21-i2c"; 
941     reg = <0x021a0000 0x4000>; 
942     interrupts = <GIC_SPI 36 IRQ_TYPE_LEVEL_HIGH>; 
943     clocks = <&clks IMX6UL_CLK_I2C1>; 
944     status = "disabled"; 
945 }; 

/* imx6ull-alientek-emmc.dts  */
224 &i2c1 { 
225     clock-frequency = <100000>; 
226     pinctrl-names = "default"; 
227     pinctrl-0 = <&pinctrl_i2c1>; 
228     status = "okay"; 
229  
230     mag3110@0e { 
231         compatible = "fsl,mag3110"; 
232         reg = <0x0e>; 
233         position = <2>; 
234     }; 
235   };
```

​		如何修改内容，理论上应该要明白修改不同内容，可以用哪些函数进行获取对应的内容，故暂时按下不表。

#### 4.4 设备树在系统中的体现

​	linux在启动的过程中会解析设备树的节点信息，并且在根节点 `/proc/device-tree`下面会根据设备树节点信息创建很多子文件夹。

#### 4.5 获取设备树中的信息

​	我们已经描述到，设备树是用来描述硬件信息的，于是在知道基本的设备树的语法之后，我们要掌握设备树获取这些硬件信息的函数。

* **查找设备树节点的OF函数：**

  在linux中，利用设备树节点`device_node`结构体来描述一个设备树节点（如下所示），我们可以通过下面这些函数来寻找设备树中有没有我们想要的节点信息。

  ```c
  /* /include/linux/of.h */
  struct device_node {
  	const char *name;
  	const char *type;
  	phandle phandle;
  	const char *full_name;
  	struct fwnode_handle fwnode;
  
  	struct	property *properties;
  	struct	property *deadprops;	/* removed properties */
  	struct	device_node *parent;
  	struct	device_node *child;
  	struct	device_node *sibling;
  	struct	kobject kobj;
  	unsigned long _flags;
  	void	*data;
  #if defined(CONFIG_SPARC)
  	const char *path_component_name;
  	unsigned int unique_id;
  	struct of_irq_controller *irq_trans;
  #endif
  };
  ```

  1. of_find_node_by_name 函数 
  2. of_find_node_by_type 函数 
  3. of_find_compatible_node 函数 
  4. of_find_matching_node_and_match 函数 
  5. of_find_node_by_path 函数 

  以上这些函数返回的类型都是`device_nodede*`类型，能够描述一个设备节点的指针信息。

* **查找父/子节点的 OF 函数：**

  ​		除了能够找到对应的设备树节点，linux 还提供了能够根据当前设备设备树节点找到对应的父/子节点的函数，以方便便利的定位。

  1.  of_get_parent 函数 
  2. of_get_next_child 函数 

* **获取硬件信息的函数：**

  ​		我们定位到具体的node节点之后，重要的是根据node节点获取需要的硬件描述信息，linux为我们提供了下面这些函数接口：

  1. **of_find_property 函数：**用于查找具体的属性。 返回值也是一个`struct property`结构体指针，用于描述node 属性下的信息。

     ```c
     struct property {
     	char	*name;
     	int	length;
     	void	*value;
     	struct property *next;
     	unsigned long _flags;
     	unsigned int unique_id;
     	struct bin_attribute attr;
     };
     ```

  2. of_property_count_elems_of_size 函数：用于获取属性中元素的数量

  3. of_property_read_u32_index 函数：用于获取属性中指定第N个 U32类型的值

  4. of_property_read_u8_array 函数 

     of_property_read_u16_array 函数

     of_property_read_u32_array 函数

     of_property_read_u16_array 函数 ：用于获取属性中 u8、u16、u32 和 u64 类型的数组数据

  5. of_property_read_u8 函数 ：用于获取属性中u8 属性的值

  6. of_property_read_string 函数 ：用于读取属性中字符串的值。

  7. of_n_addr_cells 函数 ：获取#address-cells的值

  8. of_n_size_cells 函数 ：用于获取#size-cells 属性值

  9. **of_device_is_compatible 函数 ：**获取 device_node中 compatible属性值

  10. **of_get_address 函数：**用于获取device_node中属性的address 属性值

  11. **of_translate_address 函数：**将从设备树中得到的address属性值转化为物理地址值。

  12. **of_address_to_resource 函数 ：**获取描述内存空间的resource结构体。寄存器、内存这些都可以表示为resource信息描述。如下方代码所示，实际上不同的物理意义可以用flags来区分。

      ```c
      struct resource {
      	resource_size_t start;
      	resource_size_t end;
      	const char *name;
      	unsigned long flags;
      	struct resource *parent, *sibling, *child;
      };
      ```

  13. **of_iomap 函数 ：**。of_iomap 函数本质上也是将 reg 属性中地址信息转换为虚拟地址，如果 reg 属性有多段的话，可以通过 index 参数指定要完成内存映射的是哪一段，of_iomap 函数原型如下


----

### 五.实际写一个设备树驱动的例子

#### 5.1 修改设备树文件

​	如下图，简单的在已有的设备树中添加节点：

```c
 alphaled {
  #address-cells = <1>;
  #size-cells = <1>;
  compatible = "alphaled-led";
  status = "okay";
  reg = <  0X020C406C 0X04       /* CCM_CCGR1_BASE            */ 
           0X020E0068 0X04       /* SW_MUX_GPIO1_IO03_BASE    */ 
           0X020E02F4 0X04       /* SW_PAD_GPIO1_IO03_BASE    */ 
           0X0209C000 0X04       /* GPIO1_DR_BASE             */ 
           0X0209C004 0X04 >;    /* GPIO1_GDIR_BASE           */ 
 };
```

#### 5.2 添加对应的驱动文件

​	在此工程中，需要注意的有两个点。其一是这里只是用了设备树接口来获取设备树中的硬件信息，用的还是module_init/module_exit的开发框架。其二是总体上采用的还是单片机的开发思路，控制寄存器实现对应的功能。

具体驱动文件可见 代码文件夹中的deviceTreeLed1.c，主要可以分为三个部分。

* **cdev的驱动框架：**在Linux驱动中，对于LED的控制还是采用cdev实现的，由此这里采用的还是cdev的矿建，包括初始化设备号、初始化cdev、初始化设备、创建设备类、创建设备。

* **从设备树中获取硬件信息**

  利用设备树的接口从设备树中获取硬件信息，比如能够调用接口获取到设备节点和名字，再调用数组读取接口获取到REG的值和范围，进行IO remap 之后，这样就有了寄存器的空间。

* **硬件的初始化工作**

  在获取到硬件信息之后，本质上还是要让硬件工作起来，于是我们还要了解硬件的控制流，比如设置时钟、开启IO复用、设置IO输出模式、设置IO输出电平等操作。







