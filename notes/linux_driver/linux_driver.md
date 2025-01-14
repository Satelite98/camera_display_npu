[toc]

# linux 驱动学习

###　一、字符驱动

其次本次研究的为什么叫字符设备，就是因为在数据读取的时候都是`一个一个字节`的方式来传输的。

linux 中驱动的调用接口都如下图所示：任何的驱动文件成功加载之后都会在`/dev`目录下生成一个对应的文件。

![image-20241016213210180](D:\ForStudy\camera_dispaly_npu\camera_display_npu\notes\linux_driver\Untitled.assets\image-20241016213210180.png)

#### 1.1 字符设备开发顺序

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

  

#### 1.2 设备号

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

#### 1.3 编写第一个字符程序

​		具体的程序可见代码 `/home/wxwang/imax6ull/opendv/linux_kernel/drivers/my_driver/virtual_cchrdevbase.c`，这里面需要注意的就是介绍了**printk和copytouser**的函数的使用。根据编译报错问题，注意点如下：

* `__exit`中有两个下划线
* 当你去拷贝这些函数指针的时候`int (*release) (struct inode *, struct file *);`拷贝过来之后还需要对这些指针实例化见`int chrdevbase_release(struct inode *inode, struct file *filep)`

#### 1.4 编写第一个测试程序及编译

#### 1.5 驱动文件的编译和加载

##### 1.5.1 程序编译

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

##### 1.5.2 程序加载：

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
  
  * Pinctrl 属性：
  
  * GPIO属性：

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

​		如何修改内容?需要明白修改不同内容，可以用哪些函数进行获取对应的内容，故暂时按下不表。在下面的设备树接口介绍环节一一介绍。

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

* **从设备树中获取硬件信息**

  利用设备树的接口从设备树中获取硬件信息，比如能够调用接口获取到设备节点和名字，再调用数组读取接口获取到REG的值和范围，进行IO remap 之后，这样就有了寄存器的空间。

  ```c
      /* find by node name */
      dtsled_point.nd = of_find_node_by_path("/alphaled");
      /* get  compatible from node   */
      proper  = of_find_property(dtsled_point.nd, "compatible", NULL);
      /* get status from node */
      ret  = of_property_read_string(dtsled_point.nd, "status", &str);
      /* read data type */
      ret = of_property_read_u32_array(dtsled_point.nd, "reg", regdata, 10);
  
  ```

* **硬件的初始化工作**

  在获取到硬件信息之后，本质上还是要让硬件工作起来，于是我们还要了解硬件的控制流，比如设置时钟、开启IO复用、设置IO输出模式、设置IO输出电平等操作。

  ```c
      IMX6U_CCM_CCGR1 = ioremap(regdata[0], regdata[1]); 
      SW_MUX_GPIO1_IO03 = ioremap(regdata[2], regdata[3]); 
      SW_PAD_GPIO1_IO03 = ioremap(regdata[4], regdata[5]); 
      GPIO1_DR = ioremap(regdata[6], regdata[7]); 
      GPIO1_GDIR = ioremap(regdata[8], regdata[9]); 
      /* open clk */
      val = readl(IMX6U_CCM_CCGR1); 
      val &= ~(3 << 26);  
      val |= (3 << 26);   
      writel(val, IMX6U_CCM_CCGR1); 
  
      /* set IO function / IO swap */
      writel(5, SW_MUX_GPIO1_IO03); 
      writel(0x10B0, SW_PAD_GPIO1_IO03); 
      val = readl(GPIO1_GDIR); 
      val &= ~(1 << 3);  
      val |= (1 << 3);    //
      writel(val, GPIO1_GDIR); 
      
      /* set GPIO down*/
      val = readl(GPIO1_DR); 
      val |= (1 << 3);     
      writel(val, GPIO1_DR); 
     
  ```

* **cdev的驱动框架：**在Linux驱动中，对于LED的控制还是采用cdev实现的，由此这里采用的还是cdev的矿建，包括初始化设备号、初始化cdev、初始化设备、创建设备类、创建设备。

  ```c
     /* cdev setting  */
     /* 1. get major */
    if (dtsled_point.major) {        /*  定义了设备号 */ 
      dtsled_point.devid = MKDEV(dtsled_point.major, 0); 
      register_chrdev_region(dtsled_point.devid, DTSLED_CNT,  DTSLED_NAME); 
    }else {/* 没有定义设备号 */ 
       alloc_chrdev_region(&dtsled_point.devid,0,DTSLED_CNT, DTSLED_NAME );
       dtsled_point.major = MAJOR(dtsled_point.devid);
       dtsled_point.minor = MINOR(dtsled_point.devid);
    }
    printk("dtsled major=%d,minor=%d\r\n",dtsled_point.major, dtsled_point.minor);   
    
    /* 2. initial cdev */
    dtsled_point.cdev.owner = THIS_MODULE;
    cdev_init(&dtsled_point.cdev, &dts_led_fops);
  
    /* 3.add a cdev  */
      cdev_add(&dtsled_point.cdev, dtsled_point.devid,DTSLED_CNT);
    /* 4.creat class */
      dtsled_point.class =  class_create(THIS_MODULE, DTSLED_NAME); 
      if (IS_ERR(dtsled_point.class)) { 
           return PTR_ERR(dtsled_point.class); 
      } 
  
    /* 5.create device*/
     dtsled_point.device = device_create( dtsled_point.class, NULL, dtsled_point.devid, NULL,DTSLED_NAME);
     if (IS_ERR(dtsled_point.device)) { 
       return PTR_ERR(dtsled_point.device); 
     }
  ```

### 六、pinctrl和gpio 子系统

### 6.1 Pinctrl 子系统

​		第五章是利用设备树来获取信息了，但是本质上还是利用单片机的思路来初始化寄存器。此外，linux提供了GPIO/Pinctrl子系统来方便我们初始化GPIO外设。上一章初始化外设中，我们主要对GPIO硬件进行了一下的操作：

1. 获取引脚信息

2. 设置PIN脚的复用信息

3. 设置PIN脚的电气特性、引脚上下拉、速度、驱动能力等。**需要看下具体的特性的**。

#### 6.1.1 利用pinctrl 初始化GPIO的规则及含义

采用pinctrl的方式的话，我们只需要在根据实际的情况配置PIN的初始化、复用寄存器。并且按照设备树的规则进行填写即可。

```c
/* imux6ul.dts */
iomuxc: iomuxc@020e0000 {
    compatible = "fsl,imx6ul-iomuxc";
    reg = <0x020e0000 0x4000>;
};
/*imx6ull-alientek-emmc.dts*/
&iomuxc {
	pinctrl-names = "default";
	pinctrl-0 = <&pinctrl_hog_1>;
	imx6ul-evk {
		pinctrl_hog_1: hoggrp-1 {
			fsl,pins = <
				MX6UL_PAD_UART1_RTS_B__GPIO1_IO19	    0x17059 /* SD1 CD */
				MX6UL_PAD_GPIO1_IO05__USDHC1_VSELECT	0x17059 /* SD1 VSELECT */
				MX6UL_PAD_GPIO1_IO00__ANATOP_OTG1_ID    0x13058 /* USB_OTG1_ID */
			>;
	       };
    ...
        ...
        ...
}
```

​		对于这些宏定义，在`arch/arm/boot/dts/imx6ul-pinfunc.h`中有宏定义,这五个值和驱动程序以及具体的硬件设置有关，在pinctrl的规则下，再加上要0x17259的值就会有六个值。 这六个值的含义分别见下：

**<mux_reg  config_reg input_reg mux_mode input_val config_reg_val >**

```c
#define MX6UL_PAD_UART1_RTS_B__GPIO1_IO19                         0x0090 0x031C 0x0000 0x5 0x0

pinctrl_hog_1: hoggrp-1 {
    fsl,pins = <
        0x0090 0x031C 0x0000 0x5 0x0	0x17059 /* SD1 CD */
        MX6UL_PAD_GPIO1_IO05__USDHC1_VSELECT	0x17059 /* SD1 VSELECT */
        MX6UL_PAD_GPIO1_IO00__ANATOP_OTG1_ID    0x13058 /* USB_OTG1_ID */
        >;
};

<mux_reg  config_reg input_reg mux_mode input_val config_reg_val >
```

​			所以实际上，利用pinctrl设备树我们可以对pin脚的复**用寄存器、初始化寄存器、输入寄存器**完成了初始化。其中iomux的base为`0x020e0000`，复用、初始化、输入寄存器的三个偏移分别为：90H、31CH、00H。有些外设有input_reg的值，有些外设没有input_reg的值，当作为GPIO_IO19的时候，这个外设是没有input_reg的，所以为0x00H。==这里需要补一个有input_reg的情况。==

#### 6.1.2 PinCtrl 的实现简单介绍

​	上面我们在添加的pinctrl 节点的文字描述如何在Linux 内核中处理的呢？在linux内核中，Pinctrl子节点采用的是platfrom框架描述编写的驱动，当device和driver中id匹配时，就会有probe函数执行。当我们全局搜索`imx6ul-iomuxc`字符时，就能找到imx6ul的Pinctrl的驱动工程，其驱动函数是`imx6ul_pinctrl_probe`。最后会调用`imx_pinctrl_parse_groups`完成一些寄存器的初始化。

```c
imx6ul_pinctrl_probe
    ->imx_pinctrl_probe
    	->imx_pinctrl_probe_dt
    		->imx_pinctrl_parse_functions
    			->imx_pinctrl_parse_groups
    	->pinctrl_register
```

在`imx_pinctrl_probe`函数中，会有关于`imx_pinctrl_desc`结构体的初始化工作，并且赋值一些初始化函数，在`imx_pinctrl_parse_groups`函数中，会真正的去初始化pin的一些寄存器。然后再调用`pinctrl_register`函数，该函数想Linux内核注册一个PIN控制器，可以调用已经指向的`imx_pinctrl_desc`中的`imx_pctrl_ops/imx_pmx_ops/imx_pinconf_ops`这三个结构体，里面都是一些函数初始化指针。

```c
	/*imx_pinctrl_probe*/
	imx_pinctrl_desc->name = dev_name(&pdev->dev);
	imx_pinctrl_desc->pins = info->pins;
	imx_pinctrl_desc->npins = info->npins;
	imx_pinctrl_desc->pctlops = &imx_pctrl_ops;
	imx_pinctrl_desc->pmxops = &imx_pmx_ops;
	imx_pinctrl_desc->confops = &imx_pinconf_ops;
	imx_pinctrl_desc->owner = THIS_MODULE;

	/*　imx_pinctrl_parse_groups　*/
	grp->npins = size / pin_size;
	grp->pins = devm_kzalloc(info->dev, grp->npins * sizeof(struct imx_pin),
				GFP_KERNEL);
	grp->pin_ids = devm_kzalloc(info->dev, grp->npins * sizeof(unsigned int),
				GFP_KERNEL);
	if (!grp->pins || ! grp->pin_ids)
		return -ENOMEM;

	for (i = 0; i < grp->npins; i++) {
		u32 mux_reg = be32_to_cpu(*list++);
		u32 conf_reg;
		unsigned int pin_id;
		struct imx_pin_reg *pin_reg;
		struct imx_pin *pin = &grp->pins[i];
．．．
		pin_id = (mux_reg != -1) ? mux_reg / 4 : conf_reg / 4;
		pin_reg = &info->pin_regs[pin_id];
		pin->pin = pin_id;
		grp->pin_ids[i] = pin_id;
		pin_reg->mux_reg = mux_reg;
		pin_reg->conf_reg = conf_reg;
		pin->input_reg = be32_to_cpu(*list++);
		pin->mux_mode = be32_to_cpu(*list++);
		pin->input_val = be32_to_cpu(*list++);
	}
```

#### 6.1.3在设备树中添加pinctrL节点

​	官方文档路径：`Documentation/devicetree/bindings/pinctrl/fsl,imx-pinctrl.txt`。

​	在imux6ull中，pinctrl的驱动会调用`  list = of_get_property(np, "fsl,pins", &size);`语句，由此需要在添加节点时，配置属性一定要是`fsl,pins`。由此 我们可以根据硬件电路图来添加Pinctrl 节点。在`iomuxc`的的`imx6ul-evk`子节点下添加`pinctrl_test`节点。添加完成以后如下所示： 

```c
pinctrl_test:testgrp{
	fsl,pin = <
    	MX6UL_PAD_GPIO1_IO00__GPIO1_IO00 0x1b008
    >
}
```

### 6.2 GPIO 子系统

​	pinctrl 子系统用于设置PIN脚的属性和复用关系，在真正使用的过程中,可以实现GPIO子系统的功能控制，主要有三个：

* 作为输入功能时，支持读引脚值;
* 作为输出功能时，支持输出高低电平；
* 部分 gpio 还负责接收中断；

#### 6.2.1 在设备树中添加GPIO信息

```c
/* 在GPIO 使用侧需要注明使用引脚和有效电平 */
&usdhc1 {
...
	cd-gpios = <&gpio1 19 GPIO_ACTIVE_LOW>;
...
};

/*GPIO 控制器的初始化：*/
gpio1: gpio@0209c000 {
    compatible = "fsl,imx6ul-gpio", "fsl,imx35-gpio";
    reg = <0x0209c000 0x4000>;					/* GPIO　数据寄存器起始地址 */ 
    interrupts = <GIC_SPI 66 IRQ_TYPE_LEVEL_HIGH>,
    <GIC_SPI 67 IRQ_TYPE_LEVEL_HIGH>;
    gpio-controller;　　　　/* 表示是一个GPIO控制器*/ 
    #gpio-cells = <2>;　　　/* 表示GPIO信息由两部分表述，如上　&gpio1 19表示GPIO1—19　GPIO_ACTIVE_LOW　表示低电平有效*/ 
    interrupt-controller;
    #interrupt-cells = <2>;
};
```

#### 6.2.2 GPIO子系统的引用函数

​	Pinctrl 会默认把对应的IO进行初始化完毕，由于系统会自动根据配置的选项把PIN脚初始化好，GPIO子系统是控制/读取PIN脚的电平，所以会有一些默认的接口函数供我们使用。

* **gpio_request 函数：**用于对使用的GPIO进行申请，第一个传参是GPIO的引脚标号（比如芯片的35号引脚，PIN35）。这个标号可以通过` of_get_named_gpio`这个设备树函数从设备树中直接获取。
* **gpio_free 函数 ：**根据这个PIN脚标号，释放这个GPIO的控制权
* **gpio_direction_input 函数 ：**设置PIN脚为输入
* **gpio_direction_output 函数：**设置PIN脚为输出
* **gpio_get_value 函数 ：**读取Pin脚值
* **gpio_set_value 函数 ：**设置Pine脚值。

#### 6.2.3 GPIO 子系统 设备树相关函数

​	上面的函数是GPIO的使用函数，有个关键参数就是GPIO的PIN脚号，这种类似的硬件信息，我们能通过dts的方式去获取：

* **of_gpio_named_count 函数 ：**用于统计设备树中某个设备树节点中的GPIO属性定义了几个GPIO信息，并且会把空的GPIO信息也统计进去，见下图：

  ```c
    gpios = <0   
             &gpio1 1 2 
              0 
             &gpio2 3 4>; 
  ```

* **of_gpio_count 函数 ：**和 of_gpio_named_count 函数一样，但是不同的地方在于，此函数统计的是“gpios”这个属性的 GPIO 数量，相当于上个函数的参数固定为“gpios”

* **of_get_named_gpio 函数 ：**这个函数比较**关键**了，他是取在`np`设备树节点下的名字为`propname`的第`index`个GPIO dts 使用语句的PIN脚号，有了这个号码之后，就可以利用`6.2.2`的函数去控制GPIO啦。

  ```c
  int of_get_named_gpio(struct device_node     *np, 
                         const char        *propname,     
  						int            index) 
  ```

  

#### 6.3实际PinCtrl & GPIO LED 用例

​	本实验的主要目的是使用Linux的Pinctrl和GPIO子系统接口，用于控制LED。我们会分为三部分去实现这个功能。

##### 1. 增加dts描述

​	需要根据硬件电路图增加需要控制的引脚的dts描述，比如对于LED1而言，是GPIO1_IO03的引脚，我们就需要增加对于的PInctrl节点和GPIO控制节点，如下代码所示：

```c
/* imx6ull-alientek-emmc.dts:  &iomuxc  -> imx6ul-evk 下面添加控制节点 */
pinctrl_gpio_leds: gpio-leds {
    fsl,pins = <
        MX6UL_PAD_GPIO1_IO03__GPIO1_IO03	0x17059
        >;
};
/* 在根节点下添加LED－GPIO的描述节点*/
gpioled { 
    #address-cells = <1>; 
    #size-cells = <1>; 
    compatible = "atkalpha-gpioled"; 
    pinctrl-names = "default"; 
    pinctrl-0 = <&pinctrl_gpio_leds>; 
    led-gpio = <&gpio1 3 GPIO_ACTIVE_LOW>; 
    status = "okay"; 
}; 
```

​		以上，我们就能够初始化好GPIO1_IO03，并且根据设备树文件获取到LED-GPIO的pin 脚号码，然后去控制对应的GPIO啦。

​       **注意：**要防止出现引脚多处复用的情况，所以在这个设备树中，我们还需要搜索`GPIO1_IO03`和`&gpio1 3`这两组关键字去防止同一个GPIO被多次初始化／多次复用的情况。

##### 2. 从dts中读取有效的信息

​	在驱动程序中是对GPIO子系统的操作，可以分为**根据名字找到子节点、根据子节点属性找到对应的GPIO、控制对应的GPIO三个步骤**。可见代码如下：

```C
/* 1. 找到子节点 */
 gpioled.nd = of_find_node_by_path("/gpioled");
/* 2.找到节点用的GPIOPIN脚 */
 gpioled.led_gpio = of_get_named_gpio(gpioled.nd, "led-gpio", 0); 
/* 3.控制该GPIO */
ret = gpio_direction_output(gpioled.led_gpio, 1);
```

##### 3.加入字符驱动框架，实现IO控制

​	由于本驱动是GPIO的驱动，可以将GPIO定义一个设备结构体，并且把**该结构体指针作为file 结构体的private_data，这样对不同文件操作的时候，就能找到对应的设备结构体**。

```c
/* 1.定义一个设备结构体 ，并且定义一个设备结构体*/
33  struct gpioled_dev{ 
34      dev_t devid;                /* 设备号       */ 
35      struct cdev cdev;           /* cdev       */ 
36      struct class *class;       /* 类         */ 
37      struct device *device;     /* 设备       */ 
38      int major;                  /* 主设备号     */ 
39      int minor;                  /* 次设备号     */ 
40      struct device_node  *nd;   /* 设备节点    */ 
41      int led_gpio;               /* led 所使用的 GPIO 编号        */ 
42  }; 
  	struct gpioled_dev gpioled; /* led 设备 */ 

/* 1.1 在init 函数中初始化这个设备结构体 指针。*/
127 static int __init led_init(void) 
128 { 
     	gpioled.nd = of_find_node_by_path("/gpioled"); 
    	......
	}
/* 2.在open的时候，为设备结构体赋值 */
53  static int led_open(struct inode *inode, struct file *filp) 
54  { 
55      filp->private_data = &gpioled; /* 设置私有数据 */ 
56      return 0; 
57  } 

/* 3. 在操作的时候，利用改设备结构体去找到设备，并且操作。 */
80  static ssize_t led_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt) 
81  {
    	struct gpioled_dev *dev = filp->private_data; 
     
100     return 0; 
101 } 
```



### 七、Linux 并发与竞争

​		linux是一个多任务系统，会出现并发访问共享内存区域的问题，所以我们需要对能够被多个线程访问的地方采用原子操作。这里我们可以总结两点：

1. **为什么要有原子保护？**怕被并发的其他任务中断。
2. **保护哪里？**共享资源（内存、寄存器、......）

再来回答下有哪些会出现并发访问的现象：

1. Linux中多线程访问的情况。
2. SMP 多核并行的情况
3. 抢占式并发访问，一个线程正在读写变量，被另一个线程抢占后更改。
4. 中断，一个线程正在读写变量，被中断抢占后更改。

#### 7.1 linux 中的并发处理机制

##### 7.1.1 原子访问

​	原子操作：能够实现以原子的方式去访问问题，保证对共享资源有控制权。

​	linux中提供了一些原子的API函数来实现原子操作。但是在使用原子操作之前，需要先定义一个原子变量。如下方代码所示，然后再调用原子API对原子变量进行操作，实现原子变量的控制。

```c
175 typedef struct { 
176     int counter; 
177 } atomic_t; 

/* 常见的原子API函数 */
int atomic_read(atomic_t *v) 
void atomic_set(atomic_t *v, int i)
void atomic_add(int i, atomic_t *v)    
void atomic_sub(int i, atomic_t *v)    
void atomic_inc(atomic_t *v)     
void atomic_dec(atomic_t *v)     
int atomic_dec_return(atomic_t *v)     
int atomic_inc_return(atomic_t *v)     
int atomic_sub_and_test(int i, atomic_t *v)     
int atomic_dec_and_test(atomic_t *v)     
int atomic_inc_and_test(atomic_t *v)  
int atomic_add_negative(int i, atomic_t *v)     
```

​	当我们对一个变量的bit位进行操作的时候，我们就不需要定义原子变量了，linux为我们提供了直接**操作内存**的原子bit位操作函数。

```c
void set_bit(int nr, void *p) /*将 p 地址的第 nr 位置 1。*/
void clear_bit(int nr,void *p) 
void change_bit(int nr, void *p)     
int test_bit(int nr, void *p)     
int test_and_set_bit(int nr, void *p) /*将 p 地址的第 nr 位置 1，并且返回 nr 位原来的值。 */  
int test_and_clear_bit(int nr, void *p)     
int test_and_change_bit(int nr, void *p)/* 将 p 地址的第 nr 位翻转，并且返回 nr 位原来的值。 */ 
 
```

##### 7.1.2 自旋锁

​	自旋锁定义：就是会使CPU 进入忙等待的锁，会一直尝试获取->自旋->尝试获取，不会进入睡眠状态。

​	**适用场景**：SMP条件下，操作系统管理多核的情况。多线程运行的情况。==多线程的情况，如果线程一直运行，不是不会释放吗？需要等了解线程调度之后再来回答这个问题==

​	**使用注意**：调用自旋锁的时间要尽量短，调用自旋锁到释放的过程中，不能调用能够让线程睡眠的操作。中断中不能使用自旋锁。这些原因的本质还是自旋锁会忙等待。如果A申请到自旋锁之后，A睡眠了（或者被B抢占了），这个时候B需要再申请这把锁，就会出现死锁的现象！

​	linux中的API接口：

```c
spinlock_t    lock;  //定义自旋锁 
/* 要先申请并初始化一个自旋锁 */
DEFINE_SPINLOCK(spinlock_t lock)  /*定义并初始化一个自选变量*/
int spin_lock_init(spinlock_t *lock)  /*初始化自旋锁。*/ 
void spin_lock(spinlock_t *lock)   /* 获取指定的自旋锁，也叫做加锁 */。 
void spin_unlock(spinlock_t *lock)  /* 释放指定的自旋锁。   */   
int spin_trylock(spinlock_t *lock) /*  尝试获取指定的自旋锁，如果没有获取到就返回 0 */     
int spin_is_locked(spinlock_t *lock)  /* 检查指定的自旋锁是否被获取，如果没有被获取就返回非 0，否则返回 0。 */
```

​	上文说到，在中断中不能使用自旋锁，还有一个办法就是关闭中断，不让中断抢占自旋锁的状态，linux提供了以下的API。比较推荐使用的使保存/释放 当前中断状态的API函数。

```c
void spin_lock_irq(spinlock_t *lock)  禁止本地中断，并获取自旋锁。
void spin_unlock_irq(spinlock_t *lock)  激活本地中断，并释放自旋锁。    
/* 保存中断状态，禁止本地中断，并获取自旋锁。  */
void spin_lock_irqsave(spinlock_t *lock, unsigned long flags)  
/* 将中断状态恢复到以前的状态，并且激活本地中断，释放自旋锁。  */
void  spin_unlock_irqrestore(spinlock_t *lock, unsigned long flags)  
```

##### 7.1.3 其他类型的锁

* **读写自旋锁：**允许多线程读，单线程写。具体的API中，读锁和写锁会区分开，方便API 调用。

  ```c
  /* 定义并初始化锁 */
  DEFINE_RWLOCK(rwlock_t lock)  定义并初始化读写锁 
  void rwlock_init(rwlock_t *lock)    初始化读写锁。 
  /* 读锁 */
  void read_lock(rwlock_t *lock)  获取读锁。 
  void read_unlock(rwlock_t *lock)  释放读锁。
      保存中断状态，禁止本地中断，并获取读锁。    
  void read_lock_irqsave(rwlock_t *lock,   unsigned long flags)  
  将中断状态恢复到以前的状态，并且激活本地中断，释放读锁。
  void read_unlock_irqrestore(rwlock_t *lock,  unsigned long flags) 
  /* 写锁 */
  void write_lock(rwlock_t *lock)  获取写锁。    
  void write_unlock(rwlock_t *lock)  释放写锁。     
   保存中断状态，禁止本地中断，并获取写锁。   
  void write_lock_irqsave(rwlock_t *lock, unsigned long flags)    
      将中断状态恢复到以前的状态，并且激活本地中断，释放读锁。 
  void write_unlock_irqrestore(rwlock_t *lock, unsigned long flags) 
  ```

* **顺序锁：**相对于读写锁的读写不能同时进行，顺序锁允许读写同时进行，但是不允许写并发执行。并且顺序锁不能保护指针变量，因为在写的时候指针可能失效，此时，去读取就会有意外。

##### 7.1.4 信号量

​		信号量也是同步的一种方式，相比于自旋锁，信号量有两个特点。其一是信号量能够有**信号量值**，就是同步使用这个共享资源的线程/CORE能够有一定数量。其二，信号量能够让线程**进入睡眠的状态，线程不会死等**。

​	信号量的特点是会让没有获取的线程进入睡眠状态，因此使用有以下注意点：

1. 中断中不能使用信号量，因为中断不能睡眠。
2. 如果需要频繁且短暂的同步，建议使用自旋锁，因为进入**睡眠状态并且任务调度会有较大的开销**。
3. 信号量比较适用于资源占用比较久的场景。

​	linux中的API接口：

```c
/* 定义 */
struct semaphore { 
    raw_spinlock_t      lock; 
    unsigned int        count; 
    struct list_head    wait_list; 
};
/* 使用 */
DEFINE_SEAMPHORE(name)  定义一个信号量，并且设置信号量的值为 1。
void sema_init(struct semaphore *sem, int val)  初始化信号量 sem，设置信号量值为 val。     
void down(struct semaphore *sem) 获取信号量，因为会导致休眠，因此不能在中断中使用。    
/* 尝试获取信号量，如果能获取到信号量就获取，并且返回 0。如果不能就返回非 0，并且不会进入休眠。 */
int down_trylock(struct semaphore *sem); 

/* 获取信号量，和 down 类似，只是使用 down 进入休眠状态的线程不能被信号打断。而使用此函数进入休眠以后是可以被信号打断的。 */
int down_interruptible(struct semaphore *sem)     
    
/* 使用示例 */    
struct semaphore sem;    /* 定义信号量   */ 
 
sema_init(&sem, 1);     /* 初始化信号量 */ 
 
down(&sem);         /* 申请信号量   */ 
/* 临界区 */ 
up(&sem);           /* 释放信号量   */ 
```

##### 7.1.5 互斥锁

​	互斥锁可以简单理解为值为1的信号量，即这个信号量只能被一个线程持有。因此，互斥体不能被递归调用。相较于自旋锁，互斥体能够使线程进入睡眠状态。

​	根据上面特性，互斥体的注意方式如下：

1. 中断不能调用，因为互斥锁也会进入睡眠。
2. 不能递归调用。

​	linux中的API接口：

```c
struct mutex { 
    /* 1: unlocked, 0: locked, negative: locked, possible waiters */ 
    atomic_t        count; 
    spinlock_t      wait_lock; 
}; 

DEFINE_MUTEX(name)  定义并初始化一个 mutex 变量。 
void mutex_init(mutex *lock)  初始化 mutex。    
void mutex_lock(struct mutex *lock)  获取 mutex，也就是给 mutex 上锁。如果获取不到就进休眠。     
void mutex_unlock(struct mutex *lock)  释放 mutex，也就给 mutex 解锁。     
int mutex_trylock(struct mutex *lock)  尝试获取 mutex，如果成功就返回 1，如果失败就返回 0。  
int mutex_is_locked(struct mutex *lock) 判断 mutex 是否被获取，如果是的话就返回1，否则返回 0。     
int mutex_lock_interruptible(struct mutex *lock)  使用此函数获取信号量失败进入休眠以后可以被信号打断。
    
/* 使用demo */
1 struct mutex lock;    /* 定义一个互斥体   */ 
2 mutex_init(&lock);    /* 初始化互斥体     */ 
3  
4 mutex_lock(&lock);    /* 上锁        */ 
5 /* 临界区 */ 
6 mutex_unlock(&lock);   /* 解锁         */ 
```



#### 7.2 驱动与并发的实验

​	本次实验的目的是利用Linux系统的同步机制来实现对LED灯这个资源的同步控制，能够达到一个LED资源，只有一个线程能被使用，下面利用自旋锁来介绍。

* **明确思路：**初始自旋锁的一定是在init函数中，释放自旋锁的逻辑也在exit函数中。那么打开的逻辑如何去做？我们利用一个变量表示LED的被占用状态，初始化为0表示没占用。利用自旋锁来对这个变量进行管理，每次读/写这个变量之前会先上锁，保证只有一个线程能够访问到此变量。于是我们可以在open函数中做个简单逻辑：
  1. 变量为0，表示没有线程使用，执行打开函数，变量++。
  2. 变量为1，表示有线程使用，返回busy 函数，返回。
  3. 在release函数中 先获取锁，对变量--，释放控制逻辑，释放锁 返回。
* **代码实现如下：**

```c
/* 定义增加 */
34  struct gpioled_dev{ 
35      dev_t devid;              /* 设备号             */ 
36      struct cdev cdev;         /* cdev             */ 
37      struct class *class;     /* 类               */ 
38      struct device *device;   /* 设备             */ 
39      int major;                /* 主设备号           */ 
40      int minor;                /* 次设备号           */ 
41      struct device_node  *nd; /* 设备节点          */ 
42      int led_gpio;             /* led 所使用的 GPIO 编号  */ 
43      int dev_stats;          /* 设备状态，0，设备未使用;>0,设备已经被使用 */ 
44      spinlock_t lock;          /* 自旋锁           */ 
45  }; 
struct gpioled_dev gpioled; /* led 设备         */ 

/* init 函数 */
155     spin_lock_init(&gpioled.lock); 
gpioled.dev_stats = 0;

/* open 函数 */
61      spin_lock_irqsave(&gpioled.lock, flags);  /* 上锁       */ 
62      if (gpioled.dev_stats) {                      /* 如果设备被使用了  */ 
63          spin_unlock_irqrestore(&gpioled.lock, flags); /* 解锁 */ 
64          return -EBUSY; 
65      } 
66      gpioled.dev_stats++;    /* 如果设备没有打开，那么就标记已经打开了 */ 
67      spin_unlock_irqrestore(&gpioled.lock, flags);/* 解锁 */ 

/* release 函数 */  
127     spin_lock_irqsave(&dev->lock, flags);   /* 上锁 */ 
128     if (dev->dev_stats) { 
129         dev->dev_stats--;  /* 关闭驱动文件的时候将 dev_stats 减 1 */ 
130     } 
131     spin_unlock_irqrestore(&dev->lock, flags);/* 解锁 */ 
```



### 八. 按键输入实验

​	输入案件采用的是GPIO18，也是采用Pinctrl 和 GPIO子系统来进行输入读取，具体配置可以参考第六章的实现。

### 九、Linux定时器实验

#### 9.1 内核时基简介

* **设定linux 时基频率：**内核时基就是内核代码运行时的systick，底层实现可能是CORE的中断。在`make menuconfig`中的`-> Kernel Features->Timer frequency (<choice> [=y])   `中能够配置，可见下图图像。

```c
 ┌─────────────────────── Timer frequency ───────────────────────┐
│  Use the arrow keys to navigate this window or press the      │
│  hotkey of the item you wish to select followed by the <SPACE │
│  BAR>. Press <?> for additional information about this        │
│ ┌───────────────────────────────────────────────────────────┐ │
│ │                        (X) 100 Hz                         │ │
│ │                        ( ) 200 Hz                         │ │
│ │                        ( ) 250 Hz                         │ │
│ │                        ( ) 300 Hz                         │ │
│ │                        ( ) 500 Hz                         │ │
│ │                        ( ) 1000 Hz                        │ │
│ └───────────────────────────────────────────────────────────┘ │
├───────────────────────────────────────────────────────────────┤
│                    <Select>      < Help > 
```

* **系统时钟的节拍记录**

  ​		在linux内核中，系统会用一个全局变量来记录当前时基的周期数。在32位系统中是32位，在64位系统中就是64位。这样就会有一个溢出的问题，为了减少溢出问题，在实际的比较过程中，我们一般建议用系统提供的接口来设置time事件。

  ```c
  /* linux 用来记录时基的变量 */
  76 extern u64 __jiffy_data jiffies_64; 
  77 extern unsigned long volatile __jiffy_data jiffies;  
  
  /* linux 中使用的比较函数 */ 
  time_after(unkown, known)  /* 一般unkown是jiffies，known是设定的timer值 */
  time_before(unkown, known) 
  time_after_eq(unkown, known) 
  time_before_eq(unkown, known) 
      
  /* 简单的调用demo */
   unsigned long timeout;     
   timeout = jiffies + (2 * HZ);    /* 超时的时间点 */    
   if(time_before(jiffies, timeout)) {
   /* 超时未发生 */
   }else{
   /* 超时发生 */
   }  
  ```

* **系统时钟的转化接口**

  针对`jiffies`，linux系统还提供了一套API函数来直接转化为秒等单位：

  ```c
  /* 将 jiffies 类型的参数 j 分别转换为对应的毫秒、微秒、纳秒。 */
  int jiffies_to_msecs(const unsigned long j) 
  int jiffies_to_usecs(const unsigned long j) 
  u64 jiffies_to_nsecs(const unsigned long j) 
      
  /* 将毫秒、微秒、纳秒转换为 jiffies 类型。 */
  long msecs_to_jiffies(const unsigned int m) 
  long usecs_to_jiffies(const unsigned int u) 
  unsigned long nsecs_to_jiffies(u64 n) 
  ```

  此时timout 的定义就可以表示为：

  ```c
  timeout = jiffies + msecs_to_jiffies(2000);  
  ```

* 内核delay接口

  上面的函数只是用于判断事件，如果有delay的需求就可以用下面的接口

  ```c
  /* 纳秒、微秒和毫秒延时函数 */
  void ndelay(unsigned long nsecs) 
  void udelay(unsigned long usecs) 
  void mdelay(unsigned long mseces) 
  ```

#### 9.2 内核定时器简介

​		上小结介绍了内核的时机和比较时间/delay的API函数。关于定时器，我们还会可能有定时周期性的任务。对此linux内核提供了软件的`timer_list`来给我们使用。值得注意的是，此定时器不会循环调用，如需循环调用，需要在每次的处理函数中更改定时时间。

* timer_list结构体

  ```c
  struct timer_list { 
      struct list_head entry; 
      unsigned long expires;         /* 定时器超时时间，单位是节拍数  */ 
      struct tvec_base *base;  
   
      void (*function)(unsigned long);   /* 定时处理函数          */ 
      unsigned long data;             /* 要传递给 function 函数的参数  */ 
   
      int slack; 
  }; 
  ```

* linux 提供的API函数

  ```c
  void init_timer(struct timer_list *timer) /* 初始化时钟结构体 */
  void add_timer(struct timer_list *timer)  /* 向linux 内核注册时钟结构体 */ 
  int del_timer(struct timer_list * timer)  /* 删除定时器 */    
  int del_timer_sync(struct timer_list *timer) /* 删除定时器的同步版本 */    
  int mod_timer(struct timer_list *timer, unsigned long expires) /* 设置定时时间并且激活 */    
  ```

#### 9.3 内核定时器函数demo

​		简单的定时器demo思路为：

1. 定义一个定时器实例。
2. 写一个定时器处理函数，如需循环调用，添加`mod_timer`函数每次定时函数中再初始化一次。
3. 在init函数中 初始化timer、设定timer参数、向系统中添加这个timer。
4. 在退出函数中删除此定时器。

```c
1  struct timer_list timer;  /* 定义定时器  */
4  void function(unsigned long arg) 
5  {     
    ...
        ....
13    mod_timer(&dev->timertest, jiffies + msecs_to_jiffies(2000));  
14 } 

16 /* 初始化函数 */ 
17 void init(void)   
18 { 
19    init_timer(&timer);               /* 初始化定时器      */ 
20  
21    timer.function = function;       /* 设置定时处理函数    */ 
22    timer.expires=jffies + msecs_to_jiffies(2000);/* 超时时间 2 秒 */ 
23    timer.data = (unsigned long)&dev;  /* 将设备结构体作为参数  */ 
24   
25   add_timer(&timer);             /* 启动定时器       */ 
26 } 

28 /* 退出函数 */ 
29 void exit(void) 
30 { 
31    del_timer(&timer);  /* 删除定时器 */ 
32    /* 或者使用 */ 
33    del_timer_sync(&timer); 
34 }
```



### 十、Linux中断实验

#### 10.1 硬件中断描述

* **中断向量表**

  cortexA7的中断控制也是由中断向量表实现的，具体的中断向量入口可见下图。各个中断的含义如下，软件中断系统 需要关注的主要是复位中断和IRQ/FIQ中断.

  * 复位中断：CPU 复位以后就会进入复位中断。
  * 未定义指令中断(Undefined Instruction)，如果指令不能识别的话就会产生此中断。
  * 软中断(Software Interrupt,SWI)，由 SWI 指令引起的中断，Linux 的系统调用会用 SWI指令来引起软中断，通过软中断来陷入到内核空间。这个是**CPU的软中断和GIC软中断**的概念不一致
  * 指令预取中止中断(Prefetch Abort)，预取指令的出错的时候会产生此中断。 
  * 数据访问中止中断(Data Abort)，访问数据出错的时候会产生此中断。
  * IRQ 中断(IRQ Interrupt)，核外部中断。由GIC控制器触发。 
  * FIQ 中断(FIQ Interrupt)，快速中断，如果需要快速处理中断的话就可以使用此中断。

  ![image-20241222104113952](.\linux_driver.assets\image-20241222104113952.png)

* **GIC控制器**

  ​		GIC是arm的中断控制器IP，能够对外部中断控制、设置优先级、同步给不通的CORE(多核系统)。GIC和cortexA7之间的中断通路如下所示。可见GIC会把中断信号通过FIQ/IRQ/VFIQ/VIRQ 4个通路输入给arm内核。

  ![image-20241222110253031](.\linux_driver.assets\image-20241222110253031.png)

  ​		上图左侧是GIC的中断输入来源，总体可以分为三个部分：

  * interrupt ID 0-15：SGI 软件中断，由软件写GIC寄存器触发。是分别通知多个core的。

  * interrupt ID 16-31：PPI中断（Private Peripheral Interrupt），私有外设中断，也是分别通知各个core的。

  * interrupt ID 32-1019：SPI中断（Share Peripheral Interrupt），共享外设中断，多核共用，能够通知到所有的core，让所有的core处理。

    对于GIC控制器，可以分为Distributor端和CPU interface端两个部分的工作。

    **Distributor端：**

      ①、全局中断使能控制。 
      ②、控制每一个中断的使能或者关闭。 
      ③、设置每个中断的优先级。 
      ④、设置每个中断的目标处理器列表。 
      ⑤、设置每个外部中断的触发模式：电平触发或边沿触发。 
      ⑥、设置每个中断属于组 0 还是组 1。

    **CPU interface端：**

    ①、使能或者关闭发送到 CPU Core 的中断请求信号。

    ②、应答中断。 
    ③、通知中断处理完成。 
    ④、设置优先级掩码，通过掩码来设置哪些中断不需要上报给 CPU Core。 
    ⑤、定义抢占策略。 
    ⑥、当多个中断到来的时候，选择优先级最高的中断通知给 CPU Core。  

* **CP15协处理器。**

  CP15协处理器寄存器是特殊的寄存器，有专门的指令能够进行读写，具体格式见下方:

  ```c
  /* write  register value to  CP15*/
  MCR{cond} p15, <opc1>, <Rt>, <CRn>, <CRm>, <opc2> 
  /* read  CP15 value to register */
  MRC{cond} p15, <opc1>, <Rt>, <CRn>, <CRm>, <opc2>     
  ```

  其中各部分参数解释如下：

  * cond:指令执行的条件码，如果忽略的话就表示无条件执行。 
  * opc1：协处理器要执行的操作码。 
  * Rt：ARM 源寄存器，要写入到 CP15 寄存器的数据就保存在此寄存器中。 
  * CRn：CP15 协处理器的目标寄存器。 
  * CRm：协处理器中附加的目标寄存器或者源操作数寄存器，如果不需要附加信息就将CRm 设置为 C0，否则结果不可预测。
  *  opc2：可选的协处理器特定操作码，当不需要的时候要设置为 0。

  由此，我们如果想要将CP15中的C0寄存器的值督导R0寄存器中，可以用以下的汇编函数：

  ```c
  MRC p15, 0,R0,C0,C0,0
  ```

  介绍CP15寄存器的原因是 C1寄存器作为SCTLR寄存器使用的时候，能够控制中断向量表。其中bit13 为中断向量表基地址选择，为0表示中断向量表地址是0x00000000，可以通过VBAR寄存器来重新映射。为1表示中断向量表地址是0xFFFF0000，不可以被重新映射。

  VBAR 寄存器：C12在` CRn=c12，opc1=0，CRm=c0，opc2=0 `表示VBAR寄存器，可以根据实际的链接地址文件去初始化这个链接地址。如果文件编译在0x87800000地址处，则可执行

  ```C
  ldr r0, =0X87800000      /*  r0=0X87800000  */
  MCR p15, 0, r0, c12, c0, 0   /* 将 r0 里面的数据写入到 c12 中，即 c12=0X87800000  */
  ```

#### 10.2 设备树描述

#### 10.3 中断上半部

​		在linux 中对于中断的处理分为中断上半部和下半部，中断上半部是中断ID对应的处理函数。我们要在linux中使能一个中断上半部处理函数，需要利用到下面这些API。

* request_irq：该函数用于向linux申请中断。request_irq会引起线程的休眠，所以中断中不能使用该函数。其各个参数的含义分别见下方函数注释。

  ```c
  /*
  *irq:硬件中断ID号
  *handler:中断处理函数
  *flags:一些中断处理标记,描述中断的状态，例如上升/下降沿触发、单次/多次触发等
  *name:中断名字
  *dev:传递给handler的参数， 如果是SPI（shared private interrupt）的话，需要用dev来区分不同的中断。
  */
  int request_irq(unsigned int      irq,   
       			irq_handler_t     handler,   
        			unsigned long     flags, 
            		const char   	  *name,   
      			void       		  *dev) 
  ```

* free_irq：释放中断资源，如果中断不是共享的，free_irq 会删除中断函数并且禁止中断。

  ```c
  /*
  *irq:硬件中断ID号
  *dev:传递给handler的参数， 如果是SPI（shared private interrupt）的话，需要用dev来区分不同的中断。
  */
  void free_irq(unsigned int    irq,   
    				   void       *dev) 
  ```

* 中断处理函数定义：

  ```c
  irqreturn_t (*irq_handler_t) (int, void *) 
  ```

* 使能和静止中断：

  ​		下方第一组是对于当个ID中断的使能或者禁止。但是其中的`disable_irq`会等待正在执行完的中断处理函数执行完才会返回。我们可以调用`disable_nosync`不等待同步，直接异步返回。

  ​		下方第二组是对于全局中断的操作，但是这种操作方式是对整个中断的全局操作，而且A线程关闭的中断，可以被B线程调用同样的接口打开。比如 A 设置关闭中断后delay 10S后打开，但是现在进行任务调度，B线程中利用同样的接口关闭中断，5S后打开。5S后全局中断就会被打开，这样并没有达到A线程的目的。由此我们建议使用第三组接口。

  ​		第三组接口中能够保存之前的中断现场，再disable 中断，并且在restore之后 恢复中断的现场。

  ```c
  /* 使能或禁止对应ID的中断 */
  void enable_irq(unsigned int irq) 
  void disable_irq(unsigned int irq)
  void disable_nosync(unsigned int irq)
      
  /* 使能/禁止 全局中断 */
  void local_irq_enable(void) 
  void local_irq_disable(void) 
      
  /* 保护现场的使能/禁止中断 */
  void local_irq_save(flags)   
  void local_irq_restore(flags) 
  ```

#### 10.4 中断下半部

​		linux 中中断是个特殊状态，其他线程都必须等中断中断任务执行完才能执行。所以中断中不能执行时间过久、也不能调用会引起睡眠的函数。对于有数据处理需求的，除了对实时性有要求的处理以外，其他任务我们建议放在中断的下半部去处理。linux为中断下半部提供了以下接口：

##### 10.4.1 软中断

​		==本质上的实现机制不清楚==可以参考这个链接https://blog.csdn.net/heli200482128/article/details/126078721

​		软中断是linux中的一种下半部的实现机制，本质上是一个中断指针，并且内核中定义了10个大小的软中断数组来供线程调用使用。在SMP多核结构下，不同的核心调用软中断处理函数是相同的。部分定义代码可见下方代码区。

```c
/*softirq_action定义  */
433 struct softirq_action 
434 { 
435     void    (*action)(struct softirq_action *); 
436 }; 

/* 软中断函数大小定义 */
static struct softirq_action softirq_vec[NR_SOFTIRQS]; 
/* 实际的数值表示 */
enum 
{ 
    HI_SOFTIRQ=0,             /* 高优先级软中断     */ 
    TIMER_SOFTIRQ,            /* 定时器软中断      */ 
    NET_TX_SOFTIRQ,           /* 网络数据发送软中断   */ 
    NET_RX_SOFTIRQ,           /* 网络数据接收软中断   */ 
    BLOCK_SOFTIRQ,           
    BLOCK_IOPOLL_SOFTIRQ,    
    TASKLET_SOFTIRQ,          /* tasklet 软中断     */ 
    SCHED_SOFTIRQ,            /* 调度软中断       */ 
    HRTIMER_SOFTIRQ,          /* 高精度定时器软中断  */ 
    RCU_SOFTIRQ,              /* RCU 软中断      */ 
 
    NR_SOFTIRQS 
}; 
```

​		linux中定义了全局软中断函数表，但是实际的函数内容还是可以被修改赋值的。可以调用`open_softirq`来**指定具体的软中断函数**。并且通过`raise_softirq`函数来触发软中断函数。

​		需要注意的是，软中断具体函数的赋值操作需要在**编译阶段就指定好**，即**不支持动态修改软中断处理函数指针**。

```c
void open_softirq(int nr,    void (*action)(struct softirq_action *)) 
void raise_softirq(unsigned int nr) 
    
/*实际使用例子 */
634 void __init softirq_init(void) 
635 { 
636     int cpu; 
637  
638     for_each_possible_cpu(cpu) { 
639         per_cpu(tasklet_vec, cpu).tail = 
640             &per_cpu(tasklet_vec, cpu).head; 
641         per_cpu(tasklet_hi_vec, cpu).tail = 
642             &per_cpu(tasklet_hi_vec, cpu).head; 
643     } 
644  
645     open_softirq(TASKLET_SOFTIRQ, tasklet_action); 
646     open_softirq(HI_SOFTIRQ, tasklet_hi_action); 
647 }    
```

​		软中断具有一些自己的特性：

1. 软中断是多个CPU CORE都能访问的，所以需要做好共享资源的临界区保护。
2. 软中断是等待内核线程的调度执行，软中断中不能嵌套软中断触发。
3. 软中断函数是静态编译的，在运行中部可以更改。
4. 运行在中断上下文中，不能进入**睡眠和阻塞**。

##### 10.4.2 tasklet

​		上面描述了部分软中断的特殊性，tasklet是一种特殊的软中断实现方式，同样不能被**阻塞和睡眠**，相较于软中断，taklet有下面几个特点：

1. 一种特定类型的tasklet 只能允许在一个CPU上，不同类型的taklet可以运行在不同的core上。这样减小了tasklet函数的编写难度。

2. 支持动态指定处理函数，可以在运行时指定响应函数，更方便驱动中使用该机制。

​     tasklet本质上是利用两种类型的软中断（`HI_SOFTIRQ、TASKLET_SOFTIRQ`）实现的，在实际的驱动开发中，我们会更建议使用tasklet。tasklet 在linux中的API中的接口如下，从接口也可以看出，tasklet 是一个链表。

```c
/* 结构定义 */
484 struct tasklet_struct 
485 { 
486     struct tasklet_struct *next;    /* 下一个 tasklet       */ 
487     unsigned long state;              /* tasklet 状态         */ 
488     atomic_t count;                   /* 计数器，记录对 tasklet 的引用数 */ 
489     void (*func)(unsigned long);    /* tasklet 执行的函数     */ 
490     unsigned long data;               /* 函数 func 的参数      */ 
491 };

void tasklet_init(struct tasklet_struct    *t, 
                  void (*func)(unsigned long),   
      			  unsigned long    data); 
void tasklet_schedule(struct tasklet_struct *t) /* 将对应的tasklet 加入调度队列 */
    
··
```

如果需要使用tasklet 可以按照下面的顺序实现

1. 定义以恶搞tasklet 、tasklet_func并且在模块init函数中初始化。
2. 初始化改模块的中断函数，并且在中断函数中调用`tasklet_schedule(tasklet);`以实现下半部的运行。

```c
/* 定义 taselet       */ 
struct tasklet_struct testtasklet; 
 
/* tasklet 处理函数     */ 
void testtasklet_func(unsigned long data) 
{ 
    /* tasklet 具体处理内容 */ 
} 
 
/* 中断处理函数 */ 
irqreturn_t test_handler(int irq, void *dev_id) 
{ 
    ...... 
    /* 调度 tasklet     */ 
    tasklet_schedule(&testtasklet); 
    ...... 
} 
 
/* 驱动入口函数        */ 
static int __init xxxx_init(void) 
{ 
    ...... 
    /* 初始化 tasklet     */   
    tasklet_init(&testtasklet, testtasklet_func, data); 
    /* 注册中断处理函数    */ 
    request_irq(xxx_irq, test_handler, 0, "xxx", &xxx_dev); 
    ...... 
} 
```

  ##### 10.4.3 工作队列

​        软中断和tasklet 都是在中断上下文中运行的（硬件中断处理完毕，但是还是在linux中断机制中，没有回到进程上下文环境），是不允许睡眠和阻塞的。对此，如果中断下半部的处理优先级要更弱一些，linux为我们提供了一种机制为工作队列（work_struct）中去执行。实际的执行函数是在**内核线程**中去运行的，由此可以调用睡眠/阻塞的API函数。如何选择你的下半部函数是用工作队列还是软中断/tasklet呢？可以参考下面几点：

1. 如果推后执行的任务需要睡眠，那么只能选择工作队列。
2. 如果推后执行的任务需要延时指定的时间再触发，那么使用工作队列，因为其可以利用timer延时(内核定时器实现)。
3. 如果推后执行的任务需要在一个tick之内处理，则使用软中断或tasklet，因为其可以抢占普通进程和内核线程，同时不可睡眠。
4. 如果推后执行的任务对延迟的时间没有任何要求，则使用工作队列，此时通常为无关紧要的任务

对于工作队列的使用，linux同样给我们提供了一些API函数：

关于工作队列的结构体，就是以`work_struct`组成的一个list链表及其描述。

```c
/* work 结构体 */
struct work_struct { 
    atomic_long_t data;     
    struct list_head entry;  
    work_func_t func;         /* 工作队列处理函数  */ 
}; 

/* 工作队列结构体 */
struct workqueue_struct { 
    struct list_head    pwqs;        
    struct list_head    list;        
    struct mutex        mutex;       
    int         work_color;  
    int         flush_color;     
    atomic_t        nr_pwqs_to_flush;  
    struct wq_flusher   *first_flusher;  
    struct list_head    flusher_queue;   
    struct list_head    flusher_overflow; 
    struct list_head    maydays;     
    struct worker       *rescuer;    
    int         nr_drainers;     
    int         saved_max_active;  
    struct workqueue_attrs  *unbound_attrs;  
    struct pool_workqueue   *dfl_pwq;    
    char            name[WQ_NAME_LEN];  
    struct rcu_head     rcu; 
    unsigned int        flags ____cacheline_aligned;  
    struct pool_workqueue __percpu *cpu_pwqs;  
    struct pool_workqueue __rcu *numa_pwq_tbl[];  
};
```

在linux中，每一个CPU有自己的工作线程worker，每个worker线程中有一个工作队列指针，描述工作队列中的所有工作。

```c
struct worker { 
    union { 
        struct list_head    entry;   
        struct hlist_node   hentry;  
    }; 
    struct work_struct  *current_work;   
    work_func_t     current_func;    
    struct pool_workqueue   *current_pwq;  
    bool            desc_valid;  
    struct list_head    scheduled;   
    struct task_struct  *task;       
    struct worker_pool  *pool;                       
    struct list_head    node;        
    unsigned long       last_active;     
    unsigned int        flags;       
    int          id;      
    char            desc[WORKER_DESC_LEN]; 
    struct workqueue_struct *rescue_wq;  
}; 
```

在实际的驱动处理中，我们并不需要对内核线程有管理，只需要进行work 的初始化就行了，内核会自动加入队列然后工作。利用工作队列的使用demo可见下方：

```c
/* 定义工作(work)           */ 
struct  work_struct testwork; 

void testwork_func_t(struct work_struct *work); 
{ 
    /* work 具体处理内容   */ 
} 

/* 中断处理函数        */ 
irqreturn_t test_handler(int irq, void *dev_id) 
{ 
    ...... 
    /* 调度 work          */ 
    schedule_work(&testwork); 
    ...... 
} 

/* 驱动入口函数             */ 
static int __init xxxx_init(void) 
{ 
    ...... 
    /* 初始化 work         */ 
    INIT_WORK(&testwork, testwork_func_t); 
    /* 注册中断处理函数      */ 
    request_irq(xxx_irq, test_handler, 0, "xxx", &xxx_dev); 
    ...... 
} 
```

#### 10.5 linux 中断使用demo

#### 10.6 为什么软中断和tasklet 还在中断上下文中？不是硬件中断已经处理结束了吗？

在 Linux 中，软中断（softirq）和 tasklet 是运行在中断上下文中的，而不是进程上下文。这是因为它们的设计目标就是处理高效的、无需阻塞的短任务，并且尽量减少延迟对系统的影响。让我们详细拆解一下你的疑问：

##### 10.6.1. 软中断和 tasklet 的运行上下文

* 软中断和tasklet 是内核中处理某些延迟工作的重要机制，它们运行在中断上下文中。
  * 中断上下文的特点：
    * 没有专属的进程（没有 `task_struct`）。
    * 不能进行阻塞（如睡眠、等待）。
    * 运行时必须快速完成任务，以免影响系统其他部分的响应。
  * **软中断**和 **tasklet** 共享这些特点，因此它们运行在中断上下文，而不是进程上下文。

------

##### 10.6.2. **为什么软中断和 tasklet 不在进程上下文中？**

当硬中断触发时，Linux 可能会挂起当前进程，并进入中断处理程序。在硬中断中，时间有限，因此设计了软中断机制，将部分较复杂或非紧急的任务推迟到稍后执行，以提高系统效率。

* **软中断的执行时机：**
  * 软中断是在硬中断处理程序返回后，由内核调度器安排执行的，典型场景是通过检查 `softirq` 的标志位，在返回用户态或切换上下文时执行。
  * 尽管硬中断的处理已经结束，但软中断和 tasklet 并未脱离中断上下文。它们依然是运行在内核的中断处理机制中，不会切换到特定的进程上下文。
* **避免阻塞的需求：**
  * 中断上下文下的代码执行不能引发阻塞操作（如睡眠），因为阻塞会导致整个系统性能下降，甚至死锁。
  * 因此，软中断和 tasklet 是一种轻量级的工作方式，它们在中断上下文中执行，避免切换到进程上下文以节省资源。

------

##### 10.6.3. **“退出中断” 与软中断**

软中断和 tasklet 的确是在硬中断处理完毕后开始执行的，但从内核的视角来看：

* 硬中断执行完后，进入软中断的执行时，依然视为中断上下文的一部分。
* **没有完全退出中断上下文**，因为内核还没有恢复到具体的进程上下文，也没有回到用户态。

换句话说，尽管硬中断已经完成，但软中断和 tasklet 的运行环境仍然是中断上下文，不是进程上下文。

------

##### 10.6.4. **如何理解“上下文”**

* **中断上下文**：系统在处理中断时运行的环境，与具体进程无关，无法睡眠或进行阻塞操作。
* **进程上下文**：关联到具体的用户进程，允许睡眠，可以执行阻塞操作。

软中断和 tasklet 属于中断上下文，内核为了性能考虑设计如此。如果需要阻塞操作或更复杂的处理，可以通过 **工作队列（workqueue）** 把任务推到进程上下文中处理。

------

##### 10.6.5 总结

软中断和 tasklet 的运行时机虽然在硬中断之后，但依然属于中断上下文。它们没有“退出中断”，因为其设计目标是快速处理无需阻塞的任务，而不涉及具体的进程上下文。

---





### 11. 阻塞和非阻塞IO

​	阻塞IO和非阻塞IO是应用层调用驱动的两种方式。其中，**阻塞IO**是指在调用驱动设备时，如果不能获取设备资源，就是使应用层进入阻塞状态(休眠状态)，待能获取驱动资源时，会进入调度状态。而**非阻塞IO**是指，当无法获取硬件资源时，会返回结果，应用程序可以执行别的操作。应用层调用的接口如下：

```c
4 fd = open("/dev/xxx_dev", O_RDWR);     /* 阻塞方式打开  */ 
5 ret = read(fd, &data, sizeof(data));    /* 读取数据    */ 

4 fd = open("/dev/xxx_dev", O_RDWR | O_NONBLOCK);  /* 非阻塞方式打开 */ 
5 ret = read(fd, &data, sizeof(data));          /* 读取数据 */ 
```

​	下面分别介绍了阻塞IO和非阻塞IO的驱动实现方法：

#### 11.1等待队列

​	等待队列也是linux内核提供的一种机制，能够使加入等待队列的线程进入睡眠状态（让出CPU资源）。我们可以在驱动中加入等待队列，并且让线程调用驱动时，如果没有资源，就加入等待队列，以实现阻塞IO的功能。

* **等待队列API接口：**

  1. **等待队列头：**引用等待队列需要先定义一个等待队列头，才能够使用等待队列项，下方是linux系统中等待队列头的结构体。

     ```c
     /* 示例代码 52.1.2.1 wait_queue_head_t 结构体 */ 
     39 struct __wait_queue_head { 
     40  spinlock_t      lock; 
     41  struct list_head    task_list; 
     42 }; 
     43 typedef struct __wait_queue_head wait_queue_head_t; 
     ```

  2. **等待队列项：**实际的等待队列是根据等待队列项进行调度的，我们驱动中也要将线程和对应的等待队列项绑定，然后加入等待队列中，实现阻塞IO。

     我们利用`DECLARE_WAITQUEUE(name, tsk) ` 就可以初始化一个等待队列项，其中`name`是等待队列的名字，`tsk`表示这个等待队列属于哪个进程，一般会定义为`current`。这个宏的实现就是定义了一个这个名字的等待队列，并且把`private`指针指向了当前进程。

     ```c
     /* 示例代码 52.1.2.2 wait_queue_t  结构体  */
      struct __wait_queue { 
         unsigned int          flags; 
         void                   *private; 
         wait_queue_func_t     func; 
         struct list_head      task_list; 
          }; 
      typedef struct __wait_queue wait_queue_t; 
     
     #define __WAITQUEUE_INITIALIZER(name, tsk) {				\
     	.private	= tsk,						\
     	.func		= default_wake_function,			\
     	.task_list	= { NULL, NULL } }
     
     #define DECLARE_WAITQUEUE(name, tsk)					\
     	wait_queue_t name = __WAITQUEUE_INITIALIZER(name, tsk)
     ```

  3.　**向队列中添加/移除队列项：**当资源不可访问时，我们需要将对应的队列项加入等待队列头，才能让该进程睡眠。相反，当进程不在需要此类资源后，我们需要将对应的等待队列移出链表。

     ```c
     /* 加入队列 */
     void add_wait_queue(wait_queue_head_t    *q,    
                 wait_queue_t      *wait) 
     /* 移出队列 */
     void remove_wait_queue(wait_queue_head_t    *q,   
                 wait_queue_t       *wait) 
     ```

  4.　**唤醒等待队列API：**当资源可以用的使用，需要唤醒对应等待队列中的进程。wake_up 函数可以唤醒处于 TASK_INTERRUPTIBLE 和 TASK_UNINTERRUPTIBLE 状态的进程，而 wake_up_interruptible 函数只能唤醒处于 TASK_INTERRUPTIBLE 状态的进程。 

     ```c
     void wake_up(wait_queue_head_t *q) 
     void wake_up_interruptible(wait_queue_head_t *q) 
     ```

  5.　**设置等待队列唤醒条件：** 系统中为我们留了一些设定等待队列唤醒的接口，分别如下：

     ```c
     wait_event(wq, condition)  /*等待以 wq 为等待队列头的等待队列被唤醒，前提是 condition 条件必须满足(为真)，否则一直阻塞 */
         
     wait_event_timeout(wq, condition, timeout)  /* 功能和 wait_event 类似，但是此函数可以添加超时时间，以 jiffies 为单位。此函数有返回值，如果返回 0 的话表示超时时间到，而且Condition为假。为 1 的话表示 condition 为真，也就是条件满足了。*/ 
         
     wait_event_interruptible(wq, condition) /* 与 wait_event 函数类似，但是此函数将进程设置为 TASK_INTERRUPTIBLE，就是可以被信号打断。 */
         
     wait_event_interruptible_timeout(wq, condition, timeout)/*与 wait_event_timeout 函数类似， */
     ```

     

#### 11.2 轮询

* **应用层的轮询处理接口：**

  1. select 函数 ：select 函数用于监测一组文件的读、写、异常情况，并且可以定义一个timeout 时间来实现**IO非阻塞**的处理。该函数返回0表示超时不可访问，返回-1 表示异常。

     ```c
     int select(int        nfds,    /* 所要监视的这三类文件描述集合中，最大文件描述符加 1。 */
         	fd_set      *readfds,   /* 描述符集合 */
             fd_set      *writefds, 	/* 描述符集合 */
              fd_set     *exceptfds,  /* 描述符集合 */
          	 struct timeval   *timeout) /* 等待时间 */
         
     /* fd set 结构体 */
     typedef struct {
     	unsigned long fds_bits[__FD_SETSIZE / (8 * sizeof(long))];
     } __kernel_fd_set;
     ```

     具体的应用层select 非阻塞访问的流程可见下方代码：

     ```c
     1  void main(void) 
     2  { 
     3     int ret, fd;                   /* 要监视的文件描述符     */ 
     4     fd_set readfds;               /* 读操作文件描述符集     */ 
     5     struct timeval timeout;       
     6   
     7     fd = open("dev_xxx", O_RDWR | O_NONBLOCK);   /* 非阻塞式访问 */ 
     8   
     9     FD_ZERO(&readfds);          /* 清除 readfds       */ 
     10    FD_SET(fd, &readfds);      /* 将 fd 添加到 readfds 里面 */ 
     11       
     12   
     13    timeout.tv_sec = 0; 
     14    timeout.tv_usec = 500000;   
     15       
     16    ret = select(fd + 1, &readfds, NULL, NULL, &timeout); 
     17    switch (ret) { 
     18        case 0:             /* 超时     */ 
     19            printf("timeout!\r\n"); 
     20             break; 
     21        case -1:            /* 错误     */ 
     22             printf("error!\r\n"); 
     23             break; 
     24        default:      /* 可以读取数据 */ 
     25             if(FD_ISSET(fd, &readfds)) { /* 判断是否为 fd 文件描述符 */ 
     26                 /* 使用 read 函数读取数据 */ 
     27             } 
     28             break; 
     29    }    
     30 } 
     ```

  2. poll 函数 ：poll 函数的作用和select 函数类似，只不过select 函数最大只能处理1024个文件描述符的情况，而poll 函数没有最大文件描述符的限制。其中采用了`struct pollfd `结构体来描述了要监视的文件和监视事件。

     ```c
     int poll(struct pollfd    *fds,   /* 描述要监视的文件及类型 */
           		nfds_t      nfds,   
          		 int        timeout) 
     
         
     /* struct pollfd 结构体 */   
      struct pollfd { 
       int      fd;            /*  文件描述符    */ 
       short events;        /*  请求的事件    */ 
       short revents;          /*  返回的事件    */ 
     }; 
     
     /*
         POLLIN      有数据可以读取。 
         POLLPRI      有紧急的数据需要读取。 
         POLLOUT    可以写数据。 
         POLLERR    指定的文件描述符发生错误。 
         POLLHUP    指定的文件描述符挂起。 
         POLLNVAL    无效的请求。 
         POLLRDNORM  等同于 POLLIN
     */
      
     ```

  3. epoll 函数 ：select 和poll 函数都会出现随着监听fd的事件增多，而出现效率低下的问题，而epoll适用于处理高并发的项目需求。

     ```c
     int epoll_create(int size) /* 创建个句柄 */
         
     int epoll_ctl(int          epfd,  /* epoll 句柄 */
           		  int          op,    /* ：表示要对 epfd(epoll 句柄)进行的操作 */
            		  int        fd,      /* 要监视的文件描述符 */
              	  struct epoll_event    *event) /* 要监视的事件类型 */
     ```

* **非阻塞访问时驱动底层接口：**

  以上应用层代码轮询查询 最终都会调用驱动接口的poll函数，我们需要再驱动程序中的POLL函数中调用`poll_wait`函数，`poll_wait`函数不会引起阻塞，只是会将应用程序添加到poll_table 函数中去。

  ```c
  unsigned int (*poll) (struct file *filp, struct poll_table_struct *wait) 
  
  void poll_wait(struct file * filp, wait_queue_head_t * wait_address, poll_table *p) 
      
  ```

  

#### 11.3 阻塞IO 代码实现逻辑

​		此驱动逻辑是应用层监控按键是否被按下，如果没有被按下就阻塞。其实现逻辑是在设备私有结构体中定义了一个**等待队列头部**，在read函数中会定义一个等待队列项，判断按键是否按下，如果没有按下的话，就加入等待队列。当**唤醒时候**会判断是不是由中断唤醒的，不是的话就返回错误信号，是的话就正常执行流程。

​		由此，我们需要在案件中断中，唤醒对应的等待队列。

```c
46  /* imx6uirq 设备结构体 */ 
47  struct imx6uirq_dev{ 
            ......
            ......
61      wait_queue_head_t r_wait;    /* 读等待队列头 */ 
62  }; 

/* 1. init */
118 static int keyio_init(void) 
119 { 
		......
        ......
167     /* 初始化等待队列头 */ 
168     init_waitqueue_head(&imx6uirq.r_wait); 
169     return 0; 
170 } 

/* 2. read 的时候阻塞 */
193 static ssize_t imx6uirq_read(struct file *filp, char __user *buf,  
												size_t cnt, loff_t *offt) 
194 { 
                    ......
                    ......    
 
208     DECLARE_WAITQUEUE(wait, current);  		  /* 定义一个等待队列 */ 
209     if(atomic_read(&dev->releasekey) == 0) {  /* 没有按键按下 */ 
210         add_wait_queue(&dev->r_wait, &wait);  /* 添加到等待队列头 */ 
211         __set_current_state(TASK_INTERRUPTIBLE);/* 设置任务状态 */ 
212         schedule();                             /* 进行一次任务切换 */ 
213         if(signal_pending(current)) {   		/* 判断是否为信号引起的唤醒 */ 
214             ret = -ERESTARTSYS; 
215             goto wait_error; 
216         } 
217     	__set_current_state(TASK_RUNNING);  /*设置为运行状态 */ 
218        remove_wait_queue(&dev->r_wait, &wait); /*将等待队列移除 */ 
219     } 
220     keyvalue = atomic_read(&dev->keyvalue); 
221     releasekey = atomic_read(&dev->releasekey); 
234     return 0; 
235  
236 wait_error: 
237     set_current_state(TASK_RUNNING);          /* 设置任务为运行态  */ 
238     remove_wait_queue(&dev->r_wait, &wait);   /* 将等待队列移除   */ 
239     return ret; 
240  
241 data_error: 
242     return -EINVAL; 
243 } 

/* 3.中断的时候会唤醒等待队列 */
87  void timer_function(unsigned long arg) 
88  { 
    	......
106     /* 唤醒进程 */ 
107     if(atomic_read(&dev->releasekey)) { /* 完成一次按键过程 */ 
108         /* wake_up(&dev->r_wait); */ 
109         wake_up_interruptible(&dev->r_wait); 
110     } 
    	.....
111 } 
```

#### 11.4 非阻塞IO 代码实现逻辑

​	合阻塞IO的区别是，在read函数中如果没有案件按下就直接返回值了，但是在poll驱动函数中，会调用`poll_wait`函数加入等待队列。

```c
241 unsigned int imx6uirq_poll(struct file *filp,  
struct poll_table_struct *wait) 
242 { 
243     unsigned int mask = 0; 
244     struct imx6uirq_dev *dev = (struct imx6uirq_dev *) 
		filp->private_data; 
245  
246     poll_wait(filp, &dev->r_wait, wait);    
247      
248     if(atomic_read(&dev->releasekey)) {   /* 按键按下    */ 
249         mask = POLLIN | POLLRDNORM;          /* 返回 PLLIN   */ 
250     } 
251     return mask; 
252 } 
```



### 12. 异步通知实验

​		对于阻塞IO和非阻塞IO都是需要阻塞应用层程序（或者应用层不断轮询查询状态）来实现获取驱动信息的。最好的方式是如同硬件中断那样，驱动上报应用层状态，应用层操作IO。linux提供了异步通知功能来实现此逻辑。

​		异步通知功能用**信号（signal）**这个信息来实现异步通知。其中，linux定义了31个信号来表示不同的需求（类似于软件中断量），能够响应**自定义**不同的信号处理函数。在linux 中按下`ctrl + C`和`kill -9`等软件功能，也是利用信号来实现的。

```c
34 #define SIGHUP      	  1     /* 终端挂起或控制进程终止        */ 
35 #define SIGINT         2     /* 终端中断(Ctrl+C 组合键)        */ 
36 #define SIGQUIT        3     /* 终端退出(Ctrl+\组合键)        */ 
37 #define SIGILL         4     /* 非法指令                      */ 
38 #define SIGTRAP        5     /* debug 使用，有断点指令产生    */ 
39 #define SIGABRT        6     /* 由 abort(3)发出的退出指令     */ 
40 #define SIGIOT         6     /* IOT 指令                        */ 
41 #define SIGBUS         7     /* 总线错误                      */ 
42 #define SIGFPE         8     /* 浮点运算错误                 */ 
43 #define SIGKILL        9     /* 杀死、终止进程                */ 
44 #define SIGUSR1        10    /* 用户自定义信号 1               */ 
45 #define SIGSEGV        11    /* 段违例(无效的内存段)          */ 
46 #define SIGUSR2        12    /* 用户自定义信号 2               */ 
47 #define SIGPIPE        13    /* 向非读管道写入数据            */ 
48 #define SIGALRM        14    /* 闹钟                         */ 
49 #define SIGTERM        15     /* 软件终止                      */ 
50 #define SIGSTKFLT      16    /* 栈异常                        */ 
51 #define SIGCHLD        17    /* 子进程结束                    */ 
52 #define SIGCONT        18    /* 进程继续                      */ 
53 #define SIGSTOP        19    /* 停止进程的执行，只是暂停      */ 
54 #define SIGTSTP        20    /* 停止进程的运行(Ctrl+Z 组合键)   */ 
55 #define SIGTTIN        21    /* 后台进程需要从终端读取数据    */ 
56 #define SIGTTOU        22    /* 后台进程需要向终端写数据    */ 
57 #define SIGURG         23    /* 有"紧急"数据                */ 
58 #define SIGXCPU        24    /* 超过 CPU 资源限制              */ 
59 #define SIGXFSZ        25    /* 文件大小超额               */ 
60 #define SIGVTALRM      26    /* 虚拟时钟信号               */ 
61 #define SIGPROF        27     /* 时钟信号描述               */ 
62 #define SIGWINCH       28    /* 窗口大小改变               */ 
63 #define SIGIO          29    /* 可以进行输入/输出操作      */ 
64 #define SIGPOLL        SIGIO    
66 #define SIGPWR         30    /* 断点重启                    */ 
67 #define SIGSYS         31    /* 非法的系统调用              */ 
68 #define  SIGUNUSED     31    /* 未使用信号                  */ 
```

* **应用层中的信号处理：**

  引用层中需要实现对应信号的处理函数，和指定处理函数，见`signal`函数。

  ```c
  sighandler_t signal(int signum, sighandler_t handler) 
  /* 其中 handler 是一个函数指针 */
  typedef void (*sighandler_t)(int) 
      
  ```

  我们可以写一个应用程序demo, 里面只写一个**SIGINT(ctrl + c)**函数的处理逻辑，加打印来观察现象。

  ```c
  1  #include "stdlib.h" 
  2  #include "stdio.h" 
  3  #include "signal.h" 
  4   
  5  void sigint_handler(int num) 
  6  { 
  7      printf("\r\nSIGINT signal!\r\n"); 
  8      exit(0);
  9  } 
  10  
  11 int main(void) 
  12 { 
  13     signal(SIGINT, sigint_handler); 
  14     while(1); 
  15     return 0; 
  16 }
  
  /* 实际测试 */
  执行该程序，按下ctrl+C 会有打印，但是不会结束当前进程。
  ```

* **驱动层中的信号处理：**

  1. 异步通知是通过`file_operations`中`fasync`实现的。

  2. `fasync`函数依赖于`fasync_struct`可见下方代码。并且一般再`fasync`函数中也会调用` fasync_helper `函数来初始化之前定义的`fasync_struct`结构体。可见下方代码。

     ```c
     struct fasync_struct {
     	spinlock_t		fa_lock;
     	int				magic;
     	int				fa_fd;
     	struct fasync_struct	*fa_next; /* singly linked list */
     	struct file		*fa_file;
     	struct rcu_head		fa_rcu;
     };
     
     /* fasync 函数 */
     int (*fasync) (int fd, struct file *filp, int on) 
         
     /* fasync_helper 函数 */
     int fasync_helper(int fd, struct file * filp, int on, struct fasync_struct **fapp)   
     ```

     ​		于是我们知道，调用链路为`file_operations->fasync->fasync_helper`，这样当应用层调用IO contrl 函数`fcntl(fd, F_SETFL, flags | FASYNC)`改变fasync标记时，对应函数就会执行。

     ```c
     				/* 驱动中 fasync 函数参考示例  */
     1  struct xxx_dev {  
     2     ...... 
     3     struct fasync_struct *async_queue;  /* 异步相关结构体 */  
     4  }; 
     5  
     6  static int xxx_fasync(int fd, struct file *filp, int on) 
     7  { 
     8     struct xxx_dev *dev = (xxx_dev)filp->private_data; 
     9   
     10    if (fasync_helper(fd, filp, on, &dev->async_queue) < 0) 
     11        return -EIO; 
     12    return 0; 
     13 } 
     14 
     15 static struct file_operations xxx_ops = { 
     16    ...... 
     17    .fasync = xxx_fasync, 
     18     ...... 
     19 }; 
     
     /* realease 函数要是释放掉 */
     1 static int xxx_release(struct inode *inode, struct file *filp) 
     2 { 
     3     return xxx_fasync(-1, filp, 0); /* 删除异步通知 */ 
     4 } 
     ```

     以上，应用程序调用IO contrl来处理之后，只是让应用程序进入到了能够**被异步通知使能的状态**，此时并没有异步信号产生，当具体的信号产生时，驱动中需要调用**kill_fasync** 函数来发送指定的信号。

     ```c
     /*
     *fp：要操作的 fasync_struct。
     *sig：要发送的信号。
     *band：可读时设置为 POLL_IN，可写时设置为 POLL_OUT。
     */
     void kill_fasync(struct fasync_struct **fp, int sig, int band) 
         
     ```

* **关于应用程序使用的补充：**

  上面第一段的应用程序使用介绍只是引入，要使用fasync 异步通知还需要下方流程：

  1. 注册信号处理函数 ,即调用`singal`函数。

  2. 将本应用程序的进程号告诉给内核 ` fcntl(fd, F_SETOWN, getpid())`

  3. 开启异步通知

     ```c
     flags = fcntl(fd, F_GETFL);      /*  获取当前的进程状态      */ 
     fcntl(fd, F_SETFL, flags | FASYNC);  /*  开启当前进程异步通知功能   */ 
     ```

  
  
  #### 12.1 实验case
  
    	该实验就是再按键实验中加入，如果有按键按下，就产生一个异步通知给应用层。即在中断函数加入`kill_fasync`函数，在设备结构体中加入` struct fasync_struct *async_queue;`指针。具体实现可见下方代码。
  
  ```c
  49  /* imx6uirq 设备结构体 */ 
  50  struct imx6uirq_dev{ 
  ...... 
  64      struct fasync_struct *async_queue;      /* 异步相关结构体 */ 
  65  }; 
  
  67  struct imx6uirq_dev imx6uirq;   /* irq 设备 */ 
  
  90  void timer_function(unsigned long arg) 
  91  { 
  92      unsigned char value; 
  93      unsigned char num; 
  94      struct irq_keydesc *keydesc; 
  95      struct imx6uirq_dev *dev = (struct imx6uirq_dev *)arg; 
  ......              
  109     if(atomic_read(&dev->releasekey)) {     /* 一次完整的按键过程 */ 
  110         if(dev->async_queue)  
  111             kill_fasync(&dev->async_queue, SIGIO, POLL_IN);  /* 这里调用 */
  112     } 
  121 } 
  
  269 static int imx6uirq_fasync(int fd, struct file *filp, int on) 
  270 { 
  271     struct imx6uirq_dev *dev = (struct imx6uirq_dev *) 
  							filp->private_data; 
  272     return fasync_helper(fd, filp, on, &dev->async_queue); 
  273 } 
  
  281 static int imx6uirq_release(struct inode *inode, struct file *filp) 
  282 { 
  283     return imx6uirq_fasync(-1, filp, 0); 
  284 } 
          
  286 /* 设备操作函数 */ 
  287 static struct file_operations imx6uirq_fops = { 
  288     .owner = THIS_MODULE, 
  289     .open = imx6uirq_open, 
  290     .read = imx6uirq_read, 
  291     .poll = imx6uirq_poll, 
  292     .fasync = imx6uirq_fasync, 
  293     .release = imx6uirq_release, 
  294 }; 
  ```
  
  

### 13. Platfrom 设备驱动开发

​		在linux中，由于很多板级的开发包用的IP都是同一个原厂的，这样，这些IP的驱动也就是一样的。当我们在一款SOC芯片/板级开发包使用对应的外设/板级外围设备时，我们期望只需要提供一定的设备信息，就能够正常使用。**Platfrom 总线**就是LINUX的解决思路。如下图所示，paltfrom 总线将外设分为 **驱动** 和 **设备**两个部分，当向内核注册设备/驱动时，总线会查询有没有匹配的驱动/设备，如果匹配，对应的程序就会执行。而用于匹配驱动(driver)/设备(device)的就是**总线（Platfrom）**

<img src=".\linux_driver.assets\image-20250105195037834.png" width=60%>

Linux 中利用`bus_type`来表示总线。platfrom 总线也是`bus_type`的一种。两者定义可见下方代码。可见platfrom 是实现了`bus_type`的一些函数。

```c
struct bus_type {
	const char		*name;
	const char		*dev_name;
	struct device		*dev_root;
	struct device_attribute	*dev_attrs;	/* use dev_groups instead */
	const struct attribute_group **bus_groups;
	const struct attribute_group **dev_groups;
	const struct attribute_group **drv_groups;

	int (*match)(struct device *dev, struct device_driver *drv);    /* 很重要，匹配函数 */
	int (*uevent)(struct device *dev, struct kobj_uevent_env *env);
	int (*probe)(struct device *dev);
	int (*remove)(struct device *dev);
	void (*shutdown)(struct device *dev);

	int (*online)(struct device *dev);
	int (*offline)(struct device *dev);

	int (*suspend)(struct device *dev, pm_message_t state);
	int (*resume)(struct device *dev);

	const struct dev_pm_ops *pm;

	const struct iommu_ops *iommu_ops;

	struct subsys_private *p;
	struct lock_class_key lock_key;
};
/* platform_bus_type */
struct bus_type platform_bus_type = {
	.name		= "platform",
	.dev_groups	= platform_dev_groups,
	.match		= platform_match,
	.uevent		= platform_uevent,
	.pm		= &platform_dev_pm_ops,
};

```

​	于是我们可以看出`platform_bus_type`是利用`platform_match`函数做match的，我们进一步查看这个函数，能够发现，platfrom总线提供了**四种匹配机制**：

1. 利用设备树查找设备，查看`device_driver`中的`of_match_table`和传参进来的`dev node`有没有能够匹配上的属性。

2.  ACPI 匹配方式，

3.  id_table 匹配方式，`platform_driver`结构体中有一个`id_table`的结构体，如果是用`platform_driver`定义的driver的话就会包含整个table 表格用于和设备匹配。**注意：**此方式不是用设备树的实现方式。

4.  直接比较传入driver的名字和device 的名字时候是否匹配。但是这里注意，实际上用的是`platform_device`和`device_driver`两个结构体的定义，而不是`platform_driver`的定义。

   ```c
   static int platform_match(struct device *dev, struct device_driver *drv)
   {
   	struct platform_device *pdev = to_platform_device(dev);
   	struct platform_driver *pdrv = to_platform_driver(drv);
   
   	/* When driver_override is set, only bind to the matching driver */
   	if (pdev->driver_override)
   		return !strcmp(pdev->driver_override, drv->name);
   
   	/* Attempt an OF style match first */
   	if (of_driver_match_device(dev, drv))
   		return 1;
   
   	/* Then try ACPI style match */
   	if (acpi_driver_match_device(dev, drv))
   		return 1;
   
   	/* Then try to match against the id table */
   	if (pdrv->id_table)
   		return platform_match_id(pdrv->id_table, pdev) != NULL;
   
   	/* fall-back to driver name match */
   	return (strcmp(pdev->name, drv->name) == 0);
   }
   
   ```

   

#### 13.1 Platfrom driver

platfrom driver的结构体如下所示

```c
struct platform_driver {
	int (*probe)(struct platform_device *);
	int (*remove)(struct platform_device *);
	void (*shutdown)(struct platform_device *);
	int (*suspend)(struct platform_device *, pm_message_t state);
	int (*resume)(struct platform_device *);
	struct device_driver driver;
	const struct platform_device_id *id_table;
	bool prevent_deferred_probe;
};
```

其中，device_driver也是一个很重要的结构体，见下所示：

```c
struct device_driver {
	const char		*name;
	struct bus_type		*bus;

	struct module		*owner;
	const char		*mod_name;	/* used for built-in modules */

	bool suppress_bind_attrs;	/* disables bind/unbind via sysfs */

	const struct of_device_id	*of_match_table;
	const struct acpi_device_id	*acpi_match_table;

	int (*probe) (struct device *dev);
	int (*remove) (struct device *dev);
	void (*shutdown) (struct device *dev);
	int (*suspend) (struct device *dev, pm_message_t state);
	int (*resume) (struct device *dev);
	const struct attribute_group **groups;

	const struct dev_pm_ops *pm;

	struct driver_private *p;
};
```

​		两个结构体的表示可以见上，从这里我们可以看出，`device_driver`类似于一个基类，`platform_driver`继承了`device_driver`的很多函数。这里也能和刚刚`platfrom match`函数对应起来。不同的match 路径就会执行不同的`probe`函数。

​		其中`platform_driver`结构体利用`id_table`去匹配设备，也就是会和`platform_device`对应起来使用。`id_table`具体的定义如下。而`device_driver`就是会利用`of_match_table`来匹配设备了，一般配合设备树使用。

```c
struct platform_device_id {
	char name[PLATFORM_NAME_SIZE];
	kernel_ulong_t driver_data;
};
```

​		对于驱动开发而言，利用**Platfrom bus**就是去开发driver的这些函数，其中**probe**额外的重要，因为probe函数是设备match之后会执行的函数。相当于init。

​		当我们开发好platfrom driver 之后就可以向设linux 中注册驱动了，会调用register和unregister。

```c
int platform_driver_register (struct platform_driver    *driver) 
void platform_driver_unregister(struct platform_driver *drv) 
```

​		基本的Platfrom 驱动框架如下所示，这种就是用设备树来匹配的情况啦。==但是没用driver 的probe和remove，用的是platfrom的，可能内核是判断分别执行？==

```c
   /* 设备结构体 */ 
1   struct xxx_dev{ 
2     struct cdev cdev; 
3     /* 设备结构体其他具体内容 */ 
4   }; 
5   
6   struct xxx_dev xxxdev;   /* 定义个设备结构体变量 */ 
7   
8   static int xxx_open(struct inode *inode, struct file *filp) 
9   {     
10    /* 函数具体内容 */ 
11    return 0; 
12  } 
13  
14 static ssize_t xxx_write(struct file *filp, const char __user *buf,  
		size_t cnt, loff_t *offt) 
15  { 
16    /* 函数具体内容 */ 
17    return 0; 
18  }

23  static struct file_operations xxx_fops = { 
24    .owner = THIS_MODULE, 
25    .open = xxx_open, 
26    .write = xxx_write, 
27  }; 

29 /* 
30  * platform 驱动的 probe 函数 
31  * 驱动与设备匹配成功以后此函数就会执行 
32  */ 
33  static int xxx_probe(struct platform_device *dev) 
34  {     
35    ...... 
36    cdev_init(&xxxdev.cdev, &xxx_fops); /* 注册字符设备驱动 */ 
37    /* 函数具体内容 */ 
38    return 0; 
39  } 

41  static int xxx_remove(struct platform_device *dev) 
42  { 
43    ...... 
44    cdev_del(&xxxdev.cdev);/*  删除 cdev */ 
45    /* 函数具体内容 */ 
46    return 0; 
47  } 

49 /* 匹配列表 */ 
50 static const struct of_device_id xxx_of_match[] = { 
51    { .compatible = "xxx-gpio" }, 
52     { /* Sentinel */ } 
53 }; 

55 /*  
56  * platform 平台驱动结构体 
57  */ 
58  static struct platform_driver xxx_driver = { 
59    .driver = { 
60        .name       = "xxx", 
61        .of_match_table = xxx_of_match, 
62    }, 
63    .probe      = xxx_probe, 
64    .remove     = xxx_remove, 
65  }; 
79  module_init(xxxdriver_init); 
80  module_exit(xxxdriver_exit); 
81  MODULE_LICENSE("GPL"); 
82  MODULE_AUTHOR("zuozhongkai"); 
```

**思路总结：**platfrom 设备并不是独立于字符设备、块设备、网络设备之外的驱动，只是提供了一总设备和驱动匹配的接口。本质上的驱动还是执行的对应IP的驱动。



#### 13.2 Platfrom driver

​      对于 driver 能够有两种方式能够去描述，那么对于设备信息，也会存在`device` 和`platform_device`这两种表现形式。同样的，这也是基类 和继承类的关系

```c

struct platform_device {
	const char	*name;
	int		id;
	bool		id_auto;
	struct device	dev;
	u32		num_resources;
	struct resource	*resource;  /* 表示资源 */

	const struct platform_device_id	*id_entry;
	char *driver_override; /* Driver name to force a match */

	/* MFD cell pointer */
	struct mfd_cell *mfd_cell;

	/* arch specific additions */
	struct pdev_archdata	archdata;
};

18 struct resource { 
19    resource_size_t   start; 
20    resource_size_t   end; 
21    const char     *name; 
22    unsigned long    flags; 
23    struct resource   *parent, *sibling, *child; 
24 }; 
```

同样的，当我们用`platform_device`描述完设备信息之后，也需要用接口向linux中注册。

```c
int platform_device_register(struct platform_device *pdev) 
void platform_device_unregister(struct platform_device *pdev) 
```

一个使用`platform_device`来描述设备信息的例子如下：

```c
1  /* 寄存器地址定义*/ 
2  #define PERIPH1_REGISTER_BASE    (0X20000000) /* 外设 1 寄存器首地址 */     
3  #define PERIPH2_REGISTER_BASE    (0X020E0068) /* 外设 2 寄存器首地址 */ 
4  #define REGISTER_LENGTH           4 
5   
6  /* 资源 */ 
7  static struct resource xxx_resources[] = { 
8     [0] = { 
9          .start  = PERIPH1_REGISTER_BASE, 
10        .end    = (PERIPH1_REGISTER_BASE + REGISTER_LENGTH - 1), 
11        .flags  = IORESOURCE_MEM, 
12    },   
13    [1] = { 
14        .start  = PERIPH2_REGISTER_BASE, 
15        .end    = (PERIPH2_REGISTER_BASE + REGISTER_LENGTH - 1), 
16        .flags  = IORESOURCE_MEM, 
17    }, 
18 }; 
19  
20 /* platform 设备结构体 */ 
21 static struct platform_device xxxdevice = { 
22    .name = "xxx-gpio", 
23    .id = -1, 
24    .num_resources = ARRAY_SIZE(xxx_resources), 
25    .resource = xxx_resources, 
26 }; 

28 /* 设备模块加载 */ 
29 static int __init xxxdevice_init(void) 
30 { 
31    return platform_device_register(&xxxdevice); 
32 } 
33  
34 /* 设备模块注销 */ 
35 static void __exit xxx_resourcesdevice_exit(void) 
36 { 
37    platform_device_unregister(&xxxdevice); 
38 } 
39  
40 module_init(xxxdevice_init); 
41 module_exit(xxxdevice_exit); 
42 MODULE_LICENSE("GPL"); 
43 MODULE_AUTHOR("zuozhongkai"); 
```

以上，可以发现，Linux Platfrom 就是一组提供匹配机制的接口，我们只要按照接口定义好规对于的函数，当同时注册了driver和device时，对于的驱动函数就能执行。而且`driver`和`device`还有不同的表现形式。

---

### 14. INPUT 子系统

### 15. IIC 驱动

​	可以参考这一片手册https://blog.csdn.net/weixin_42031299/article/details/125610751

​	IIC 设备的枚举例子：https://www.cnblogs.com/downey-blog/p/10493216.html

​	linux中bus 总线的参考：https://www.cnblogs.com/downey-blog/p/10507703.html 

​	初步来看，要想实现IIC驱动设备，我们需要关心三个层次：IIC 核心层、IIC适配器驱动、IIC设备驱动。其中IIC核心层是IIC_BUS 的实现，IIC适配器是IIC控制器驱动的实现，IIC设备是具体外围设备的实现。

#### 15.1 linux bus 类 总线、驱动、设备需要的函数

* **bus_type 的总线接口 ：**如下方所示，对于bus_type 类型的 总线结构体，只需要提供下面这些函数就可以了，不需要提供匹配表。其中`name`应该是个关键属性

  ```c
  struct bus_type i2c_bus_type = {
  	.name		= "i2c",
  	.match		= i2c_device_match,
  	.probe		= i2c_device_probe,
  	.remove		= i2c_device_remove,
  	.shutdown	= i2c_device_shutdown,
  };
  ```

  ​		首先，就会有一个问题，这个bus 怎么被注册的？match 函数和probe 函数什么时候被执行的？首先在Linux的启动阶段，会有一个**linux initcall**机制，就是将特定的代码段编译放到特定的地址，然后依次执行。在那之中就会调用`i2c_init`,由此`bus_register(&i2c_bus_type);`就会执行。在其中，就会注册一条`i2c`名字的总线。

  ```c
  static int __init i2c_init(void)
  {
  ......
  	retval = bus_register(&i2c_bus_type);
  ......
  }
  ```

* **driver 需要的接口：**如下所示，可以看到`i2c_driver `类型也和paltfrom 总线的driver类似，但是有一些自己的特殊接口。从`match`函数角度来看的话是一样的，也是通过`id_table`和`device_driver->acpi_match_table` 这两个表来实现设备的匹配的。如果匹配了，该函数就会执行。

  ```c
  struct i2c_driver {
  	unsigned int class;
  	int (*attach_adapter)(struct i2c_adapter *) __deprecated;
  
  	int (*probe)(struct i2c_client *, const struct i2c_device_id *);
  	int (*remove)(struct i2c_client *);
  	void (*shutdown)(struct i2c_client *);
  	void (*alert)(struct i2c_client *, unsigned int data);
  	int (*command)(struct i2c_client *client, unsigned int cmd, void *arg);
  
  	struct device_driver driver;
  	const struct i2c_device_id *id_table;
  	int (*detect)(struct i2c_client *, struct i2c_board_info *);
  	const unsigned short *address_list; 
  	struct list_head clients;
  };
  
  /* 5640 driver ./drivers/media/platform/mxc/capture/ov5640.c*/
  static struct i2c_driver ov5640_i2c_driver = {
  	.driver = {
  		  .owner = THIS_MODULE,
  		  .name  = "ov564x",
  		  },
  	.probe  = ov5640_probe,
  	.remove = ov5640_remove,
  	.id_table = ov5640_id,
  };
  ```

* **device 需要的接口：**在linux 中，IICbus 用`i2c_client`结构体来描述IIC的设备信息。所以如果是手写设备信息的形式，就是要提供一个`i2c_client`的实现了，当然，也可以采用设备树的形式来实现一个设备。

  这里要关心两个点：

  1. i2c_client设备是怎么和driver匹配的？
  2. 设备树的形式是怎么和driver匹配的？

  ```c
  struct i2c_client {
  	unsigned short flags;		/* div., see below		*/
  	unsigned short addr;		/* chip address - NOTE: 7bit	*/
  					/* addresses are stored in the	*/
  					/* _LOWER_ 7 bits		*/
  	char name[I2C_NAME_SIZE];
  	struct i2c_adapter *adapter;	/* the adapter we sit on	*/
  	struct device dev;		/* the device structure		*/
  	int irq;			/* irq issued by device		*/
  	struct list_head detected;
  #if IS_ENABLED(CONFIG_I2C_SLAVE)
  	i2c_slave_cb_t slave_cb;	/* callback for slave mode	*/
  #endif
  };
  

    /* 设备树的形式  ./arch/arm/boot/dts/imx6ul.dtsi*/
          i2c1: i2c@021a0000 {
            #address-cells = <1>;
              #size-cells = <0>;
            compatible = "fsl,imx6ul-i2c", "fsl,imx21-i2c";
              reg = <0x021a0000 0x4000>;
              interrupts = <GIC_SPI 36 IRQ_TYPE_LEVEL_HIGH>;
              clocks = <&clks IMX6UL_CLK_I2C1>;
              status = "disabled";
          };
  ```
  
  #### match 和probe 是如何执行的？
  
  ​		在我们利用linux bus 执行的时候，我们总知道 当device 和driver 由于match 设备中的一些匹配规则，匹配了之后，我们函数的probe就会执行，但是这条通讯链路是如何调用的，我们就利用IIC 总线浅浅分析一下：
  
  ​		假设我们是用设备树的方式，即IIC device已经被注册到Linux 系统中了，此时，我们将驱动加载到系统中，函数调用关系如下，这里我们还是用ov5640的IIC 接口来阅读，下面是0V5640的IIC init 接口
  
  ```c
  static struct i2c_driver ov5640_i2c_driver = {
  	.driver = {
  		  .owner = THIS_MODULE,
  		  .name  = "ov564x",
  		  },
  	.probe  = ov5640_probe,
  	.remove = ov5640_remove,
  	.id_table = ov5640_id,
  };
  ```
  
  module_i2c_driver(ov5640_i2c_driver);
```
  
继续追踪`module_i2c_driver(ov5640_i2c_driver);`这个宏的实现,还是一个宏。
  
  ​```c
  module_i2c_driver(ov5640_i2c_driver);
  /*  宏module_i2c_driver 的实现  */
  #define module_i2c_driver(__i2c_driver) \
  	module_driver(__i2c_driver, i2c_add_driver, \
  			i2c_del_driver)
  
  module_driver(ov5640_i2c_driver,i2c_add_driver,i2c_del_driver)
```
  
我们再继续查看该宏的实现，由此，我们可以知道，最后的函数为：
  
  ```c
  /*  宏module_driver的实现 */
  #define module_driver(__driver, __register, __unregister, ...) \
  static int __init __driver##_init(void) \
  { \
  	return __register(&(__driver) , ##__VA_ARGS__); \
  } \
  module_init(__driver##_init); \
  static void __exit __driver##_exit(void) \
  { \
  	__unregister(&(__driver) , ##__VA_ARGS__); \
  } \
  module_exit(__driver##_exit);
  
  
  static int __init ov5640_i2c_driver_init(void)
  {
  	return i2c_add_driver(&(ov5640_i2c_driver));
  }
  module_init(ov5640_i2c_driver_init);
  
  static void __exit ov5640_i2c_driver_exit(void) 
  { \
  	i2c_del_driver(&(ov5640_i2c_driver)); \
  } \
  module_exit(ov5640_i2c_driver_exit);
  
```
  
由此，我们可以知道，最后调用的是**i2c_add_driver(&(ov5640_i2c_driver));**这样一个接口，我们看下这个接口会做什么内容呢？
  
  ```c
  #define i2c_add_driver(driver)  	i2c_register_driver(THIS_MODULE, driver)
  -> i2c_register_driver(THIS_MODULE，&(ov5640_i2c_driver));
  
  /* 注册总线 */
  int i2c_register_driver(struct module *owner, struct i2c_driver *driver)
  {
  ......
  	driver->driver.owner = owner;
  	driver->driver.bus = &i2c_bus_type;
  	res = driver_register(&driver->driver);
  .....
  
  	return 0;
  }
  /* 查找总线驱动 ,然后把驱动添加到总线*/
  int driver_register(struct device_driver *drv)
  {
  ......
  	other = driver_find(drv->name, drv->bus);
  	if (other) {
  		printk(KERN_ERR "Error: Driver '%s' is already registered, "
  			"aborting...\n", drv->name);
  		return -EBUSY;
  	}
  
  	ret = bus_add_driver(drv);
  .......
  }
  
  /* 会把驱动 添加到驱动链表中 ，并且执行attach 函数*/
  int bus_add_driver(struct device_driver *drv)
  {
  .....
  	struct bus_type *bus;
  	struct driver_private *priv;
  	int error = 0;
  
  	bus = bus_get(drv->bus);
  
  	klist_init(&priv->klist_devices, NULL, NULL);
  	priv->driver = drv;
  	drv->p = priv;
  	priv->kobj.kset = bus->p->drivers_kset;
     	/* 把 bus driver 加入到klist_drivers dirver 链表里面去。 */
  	klist_add_tail(&priv->knode_bus, &bus->p->klist_drivers); 
  	if (drv->bus->p->drivers_autoprobe) {
  		error = driver_attach(drv);           /*  driver attatch */
  		if (error)
  			goto out_unregister;
  	}
  	module_add_driver(drv->owner, drv);
  .....
  }
```
  
​		由上面分析可知，当一个新的driver init执行后，会执行到bus 的 `driver_attach`函数，我们再来分析分析这个函数。
  
  ```c
  int driver_attach(struct device_driver *drv)
  {
  	return bus_for_each_dev(drv->bus, NULL, drv, __driver_attach);
  }
  
  /* 由此可见，这个函数就是遍历找到device 然后data 一起作为传参给到  __driver_attach 函数*/
  int bus_for_each_dev(struct bus_type *bus, struct device *start,
  		     void *data, int (*fn)(struct device *, void *))
  {
  	struct klist_iter i;
  	struct device *dev;
  	int error = 0;
  
  	if (!bus || !bus->p)
  		return -EINVAL;
  
  	klist_iter_init_node(&bus->p->klist_devices, &i,
  			     (start ? &start->p->knode_bus : NULL));
  	while ((dev = next_device(&i)) && !error)
  		error = fn(dev, data);
  	klist_iter_exit(&i);
  	return error;
  }
  
  /*可见这个函数是关键函数，里面会先执行match 函数，然后再执行 probe 函数，我们来分别看下 */
  static int __driver_attach(struct device *dev, void *data)
  {
  	struct device_driver *drv = data;
  
  	if (!driver_match_device(drv, dev))
  		return 0;
  
  	if (dev->parent)	/* Needed for USB */
  		device_lock(dev->parent);
  	device_lock(dev);
  	if (!dev->driver)
  		driver_probe_device(drv, dev);
  	device_unlock(dev);
  	if (dev->parent)
  		device_unlock(dev->parent);
  
  	return 0;
  }
  
  /* match -会执行总线的match 函数 */
  static inline int driver_match_device(struct device_driver *drv,
  				      struct device *dev)
  {
  	return drv->bus->match ? drv->bus->match(dev, drv) : 1;
  }
  
  /* probe */
  int driver_probe_device(struct device_driver *drv, struct device *dev)
  {
  ......
  
  	pm_runtime_barrier(dev);
  	ret = really_probe(dev, drv);
  	pm_request_idle(dev);
  .....
  }
  static int really_probe(struct device *dev, struct device_driver *drv)
  {
  .......
  	if (dev->bus->probe) {
  		ret = dev->bus->probe(dev);
  		if (ret)
  			goto probe_failed;
  	} else if (drv->probe) {
  		ret = drv->probe(dev);
  		if (ret)
  			goto probe_failed;
  	}
  ......
  }
```
  
由上我们可以知道，`__driver_attach`里面会做两个事情，一是调用bus 的match 行数去匹配driver和的device。另外一个是会调用bus的probe函数，如果没有的画就会直接执行driver的probe 函数。
  

  
**关于IIC／bus驱动的局部总结** 
  
  1. 这里介绍的IICbus 驱动不是 对于SOC 上IP而言的，而是对于一个外设控制器而言的driver和device。
  2. IIC bus 负责提供match 和 probe函数，提供匹配机制，但是不必提供具体匹配内容。
  3. driver 和device 需要提供  id/match table表，并且提供具体的 driver 函数的实现。
  4. match 和probe的执行函数时机是从宏定义`module_i2c_driver(ov5640_i2c_driver);`函数开始的，里面会展开为`ov5640_i2c_driver_init`函数。 在这个函数中会调用`klist_add_tail()`函数加入到bus 的`klist_drivers`链表里面去，然后调用`driver_attach()`函数去遍历匹配driver和device。匹配过程中先是执行`bus`的match函数，然后执行`bus`的`probe`函数。如果 bus 没有probe函数的话，就会执行driver的probe函数。

#### 15.2 利用imax6ull + OV 5640 来看整个IIC驱动的流程。

 		上面描述了IIC总线的bus、driver、device的匹配过程，但是在整个IIC通讯过程中，实际只介绍了IIC设备这一侧，对于IIC控制这一侧没有介绍，下面我们将从IIC控制器开始，探究整个过程。

对于IIC的讨论，我们要抓住三个问题，

1. 你想要用哪个 I2C控制器来使用I2c 总线？

   linux 会为每一个IIC控制器建立一个IIC bus 

2. 你想要和哪个IIC 设备通讯？

3. 你要传输的数据是什么？

4. IIC_client 这个设备结构，是怎么和IIC_adapter 绑定的？

   参考这个信息：https://blog.csdn.net/qq_17270067/article/details/107233760

   * 首先，要利用platfrom 设备完成I2C_adapter 总线的注册。内核会为每一个IIC_adapter创建一个设备节点。

     ```c
     int i2c_add_adapter(struct i2c_adapter *adapter);
     ```

   * 然后内核会解析设备树上挂载到这个总线上的设备节点，IIC_client这个设备结构体，

     ```c
     i2c_new_client_device()
     ```

   * 当driver 注册的时候，会根据`of_match_table`和`compatible`匹配的属性来调用驱动`probe`函数。

     ```c
     
     ```




所以 我们现在来回顾一下，可以分为下面两个过程：

##### IIC总线控制器的初始化及设备节点的枚举。

* 设备树信息

  ```c
  /* I2C adpter */
  
  i2c2: i2c@021a4000 {
      #address-cells = <1>;
      #size-cells = <0>;
      compatible = "fsl,imx6ul-i2c", "fsl,imx21-i2c";
      reg = <0x021a4000 0x4000>;
      interrupts = <GIC_SPI 37 IRQ_TYPE_LEVEL_HIGH>;
      clocks = <&clks IMX6UL_CLK_I2C2>;
      status = "disabled";
  };
  
  /* 5640 */
  &i2c2 {
    ......
  	clock_frequency = <100000>;
  	pinctrl-names = "default";
  	pinctrl-0 = <&pinctrl_i2c2>;
  	status = "okay";
  
  	ov5640: ov5640@3c {
  		compatible = "ovti,ov5640";
  		reg = <0x3c>;
  		pinctrl-names = "default";
  		pinctrl-0 = <&pinctrl_csi1
  			     &csi_pwn_rst>;
  		clocks = <&clks IMX6UL_CLK_CSI>;
  		clock-names = "csi_mclk";
  		pwn-gpios = <&gpio1 4 1>;
  		rst-gpios = <&gpio1 2 0>;
  		csi_id = <0>;
  		mclk = <24000000>;
  		mclk_source = <0>;
  		status = "okay";
  		port {
  			ov5640_ep: endpoint {
  				remote-endpoint = <&csi1_ep>;
  			};
  		};
  	};
  ......
  }
  ```

* I2c adapter 控制器的识别过程，由设备树可以看出，其设备compatible是`imx21-i2c`，通过这个我们可以找到这个i2c 控制器的driver。

  ```c
  /* 将设备树转换成platform_device后i2c_imx_probe函数被调用 */
  i2c_imx_probe
      i2c_add_numbered_adapter /* 添加I2C控制器 */
          __i2c_add_numbered_adapter
              i2c_register_adapter /* 注册I2C控制器 */
                  device_register /* I2C控制器设备注册 */
                  of_i2c_register_devices /* 查找设备树控制器下面的从设备 */
                      of_i2c_register_device /*解析设备树属性*/
                          i2c_new_device
                              client->dev.bus = &i2c_bus_type;
                              device_register /* 添加设备I2C从设备 */
                  i2c_scan_static_board_info /* 查找静态表，有些I2C设备是在代码中写死的，不是通过设备树的形式 */
                      i2c_new_device
                          client->dev.bus = &i2c_bus_type;
                          device_register /* 添加设备I2C从设备 */
  ```

  在这个过程中，会遍历调用到`of_i2c_register_device`这个函数，会遍历设备树的IIC 从节点信息，然后注册设备。之后会调用`i2c_new_device` 来创建一个IIC 设备。

  ```c
  static void of_i2c_register_devices(struct i2c_adapter *adap)
  {
  	struct device_node *node;
  
  	/* Only register child devices if the adapter has a node pointer set */
  	if (!adap->dev.of_node)
  		return;
  
  	dev_dbg(&adap->dev, "of_i2c: walking child nodes\n");
  
  	for_each_available_child_of_node(adap->dev.of_node, node)
  		of_i2c_register_device(adap, node);
  }
  
  static struct i2c_client *of_i2c_register_device(struct i2c_adapter *adap,
  						 struct device_node *node)
  {
  	struct i2c_client *result;
  	struct i2c_board_info info = {};
  	struct dev_archdata dev_ad = {};
  	const __be32 *addr;
  	int len;
  
  	dev_dbg(&adap->dev, "of_i2c: register %s\n", node->full_name);
  
  	if (of_modalias_node(node, info.type, sizeof(info.type)) < 0) {
  		dev_err(&adap->dev, "of_i2c: modalias failure on %s\n",
  			node->full_name);
  		return ERR_PTR(-EINVAL);
  	}
  
  	addr = of_get_property(node, "reg", &len);
  	if (!addr || (len < sizeof(int))) {
  		dev_err(&adap->dev, "of_i2c: invalid reg on %s\n",
  			node->full_name);
  		return ERR_PTR(-EINVAL);
  	}
  
  	info.addr = be32_to_cpup(addr);
  	if (info.addr > (1 << 10) - 1) {
  		dev_err(&adap->dev, "of_i2c: invalid addr=%x on %s\n",
  			info.addr, node->full_name);
  		return ERR_PTR(-EINVAL);
  	}
  
  	info.of_node = of_node_get(node);
  	info.archdata = &dev_ad;
  
  	if (of_get_property(node, "wakeup-source", NULL))
  		info.flags |= I2C_CLIENT_WAKE;
  
  	result = i2c_new_device(adap, &info);
  	if (result == NULL) {
  		dev_err(&adap->dev, "of_i2c: Failure registering %s\n",
  			node->full_name);
  		of_node_put(node);
  		return ERR_PTR(-EINVAL);
  	}
  	return result;
  }
  ```

  

##### IIC外设驱动的匹配及初始化





