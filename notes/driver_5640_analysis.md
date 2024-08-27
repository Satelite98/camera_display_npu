[toc]

### 1. linux 下的IIC 驱动

#### 总结

1. 适配器的toc驱动,需要定义对应的`platfrom_device_id`和`platform_driver`的匹配驱动,来初始化芯片芯片硬件IIC控制器.

   ```c
   static struct platform_device_id imx_i2c_devtype[] = {
   	{
   		.name = "imx1-i2c",
   		.driver_data = (kernel_ulong_t)&imx1_i2c_hwdata,
   	}, {
   		.name = "imx21-i2c",
   		.driver_data = (kernel_ulong_t)&imx21_i2c_hwdata,
   	}, {
   		/* sentinel */
   	}
   };
   
   
   static struct platform_driver i2c_imx_driver = {
   	.probe = i2c_imx_probe,
   	.remove = i2c_imx_remove,
   	.driver	= {
   		.name = DRIVER_NAME,
   		.owner = THIS_MODULE,
   		.of_match_table = i2c_imx_dt_ids,
   		.pm = IMX_I2C_PM,
   	},
   	.id_table	= imx_i2c_devtype,
   };
   ```

2. 提供硬件传输算法和支持的类型,即master传输的时候的处理.

   ```c
   static struct i2c_algorithm i2c_imx_algo = {
   	.master_xfer	= i2c_imx_xfer,
   	.functionality	= i2c_imx_func,
   };
   ```

3. 提供slave 设备接口IIC 总线上的device 和driver 函数,并且需要在`dts`中设定好IIC节点,由此可以转化为`I2C_client` 结构体,由此就可以实现IIC设备的读写，最后的设备OPS接口(功能接口)可以根据具体的设备功能开发.

   ```c
   static const struct i2c_device_id ov5640_id[] = {
   	{"ov5640", 0},
   	{},
   };
   
   /* 这么没有提供 IIC接口的OPS!只有v4l2接口的OPS,IIC只做控制器 */
   static struct i2c_driver ov5640_i2c_driver = {
   	.driver = {
   		  .owner = THIS_MODULE,
   		  .name  = "ov5640",
   		  },
   	.probe  = ov5640_probe,
   	.remove = ov5640_remove,
   	.id_table = ov5640_id,
   };
   
   /* 下面是关于I2c 子节点下面的定义  */
   &i2c2 {
   	clock_frequency = <100000>;
   	pinctrl-names = "default";
   	pinctrl-0 = <&pinctrl_i2c2>;
   	status = "okay";
   
   	codec1: wm8960@1a {
   		compatible = "wlf,wm8960";
   		reg = <0x1a>;
   		clocks = <&clks IMX6UL_CLK_SAI2>;
   		clock-names = "mclk";
   		wlf,shared-lrclk;
   	};
   
   #if ATK_CAMERA_ON
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
   			camera_ep: endpoint {
   				reote-endpoint = <&csi_ep>;
   			};
   		};
   	};
   #endif
   ```

####  1.1 设备控制器: IIC-probe 函数的作用

* 定义了一个`imx_i2c_struct`结构体，是设备控制器的抽象，并且申请内存，完成这个对象的初始化。实际上完成的就是完成SOC上IIC 硬件IP的初始化。

```c
struct imx_i2c_struct *i2c_imx;
......
i2c_imx = devm_kzalloc(&pdev->dev, sizeof(*i2c_imx), GFP_KERNEL);

/* Setup i2c_imx driver structure */
strlcpy(i2c_imx->adapter.name, pdev->name, sizeof(i2c_imx->adapter.name));
i2c_imx->adapter.owner		= THIS_MODULE;
i2c_imx->adapter.algo		= &i2c_imx_algo;
i2c_imx->adapter.dev.parent	= &pdev->dev;
i2c_imx->adapter.nr		= pdev->id;
i2c_imx->adapter.dev.of_node	= pdev->dev.of_node;
i2c_imx->base			= base;

/* Get I2C clock */
i2c_imx->clk = devm_clk_get(&pdev->dev, NULL);

/* Add I2C adapter-还要注册IIC　控制器　－－　why? */
ret = i2c_add_numbered_adapter(&i2c_imx->adapter);
    
```

* IIC 的算法模块`i2c_imx_xfer`是实际的硬件传输单元，`i2c_imx_func`是返回标志位，表明支持什么功能。

```c
static struct i2c_algorithm i2c_imx_algo = {
	.master_xfer	= i2c_imx_xfer,
	.functionality	= i2c_imx_func,
};
```

* 关于`i2c_imx_xfer`会有对寄存器操作的接口，需要进一步看逻辑寄存器的操作

```c
	/* Start I2C transfer */
result = i2c_imx_start(i2c_imx);

......
temp = imx_i2c_read_reg(i2c_imx, IMX_I2C_I2CR);
temp |= I2CR_RSTA;
imx_i2c_write_reg(temp, i2c_imx, IMX_I2C_I2CR);
result =  i2c_imx_bus_busy(i2c_imx, 1);

......
/* Stop I2C transfer */
i2c_imx_stop(i2c_imx);

```

#### 1.2 IIC 设备的驱动开发

1. 需要先利用设备树，在I2Cl 的节点下面创建子节点，imx6uLL的`dts`已经分配了OV5640IIC 的信息了，其中`@`后面的`3C`是IIC器件的地址。`reg`属性也可以代表地址

   ```c
   &i2c2     
   	clock_frequency = <100000>;
   	pinctrl-names = "default";
   	pinctrl-0 = <&pinctrl_i2c2>;
   	status = "okay";
   
   	codec: wm8960@1a {
   		compatible = "wlf,wm8960";
   		reg = <0x1a>;
   		clocks = <&clks IMX6UL_CLK_SAI2>;
   		clock-names = "mclk";
   		wlf,shared-lrclk;
   	};
   
   	ov5640: ov5640@3c {
   		compatible = "ovti,ov5640";
   		reg = <0x3c>;
   		pinctrl-names = "default";
   		pinctrl-0 = <&pinctrl_csi1>;
   		clocks = <&clks IMX6UL_CLK_CSI>;
   		clock-names = "csi_mclk";
   		pwn-gpios = <&gpio_spi 6 1>;
   		rst-gpios = <&gpio_spi 5 0>;
   		csi_id = <0>;
   		mclk = <24000000>;
   		mclk_source = <0>;
   		status = "disabled";
   		port {
   			ov5640_ep: endpoint {
   				remote-endpoint = <&csi1_ep>;
   			};
   		};
   	};
   };
   ```

   同时在OV5640.C 里面还有一些IIC 的信息，分别定义了IIC总线设备和驱动，这样在match的时候，就能根据具体的OV5640 的IIC设备执行driver 函数了。 

   ```c
   /*定义了设备*/
   static const struct i2c_device_id ov5640_id[] = {
   	{"ov5640", 0},
   	{},
   };
   
   /*定义了总线驱动-driver*/
   static struct i2c_driver ov5640_i2c_driver = {
   	.driver = {
   		  .owner = THIS_MODULE,
   		  .name  = "ov5640",
   		  },
   	.probe  = ov5640_probe,
   	.remove = ov5640_remove,
   	.id_table = ov5640_id,
   };
   
   ```

   看下OV5640 IIC的probe函数都做了哪些内容：

   ```c
   static int ov5640_probe(struct i2c_client *client,
   			const struct i2c_device_id *id)
   {
   	struct pinctrl *pinctrl;
   	struct device *dev = &client->dev;
   	int retval;
   	u8 chip_id_high, chip_id_low;
   
   	/* ov5640 pinctrl - GPIO 设置 */
    
   
   	/* request power down pin - */
   
   
   	/* request reset pin -reset*/
   
   
   	/* Set initial values for the sensor struct.-读取设备树结构，获取设备树信息,并且设置时钟 */
   	memset(&ov5640_data, 0, sizeof(ov5640_data));
   	ov5640_data.sensor_clk = devm_clk_get(dev, "csi_mclk");
   	if (IS_ERR(ov5640_data.sensor_clk)) {
   		dev_err(dev, "get mclk failed\n");
   		return PTR_ERR(ov5640_data.sensor_clk);
   	}
   
   	retval = of_property_read_u32(dev->of_node, "mclk",
   					&ov5640_data.mclk);                 //获取OV5640设备树时钟
   	if (retval) {
   		dev_err(dev, "mclk frequency is invalid\n");
   		return retval;
   	}
   
   	retval = of_property_read_u32(dev->of_node, "mclk_source",
   					(u32 *) &(ov5640_data.mclk_source)); //获取OV5640时钟源
   	if (retval) {
   		dev_err(dev, "mclk_source invalid\n");
   		return retval;
   	}
   
   	retval = of_property_read_u32(dev->of_node, "csi_id",
   					&(ov5640_data.csi));                 //获取CSI ID
   	if (retval) {
   		dev_err(dev, "csi_id invalid\n");
   		return retval;
   	}
   
   	/* Set mclk rate before clk on ----设置时钟，打开时钟*/
   	ov5640_set_clk_rate();
   
   	clk_prepare_enable(ov5640_data.sensor_clk);
   
   	ov5640_data.io_init = ov5640_reset;          //对全局变量OV5640_data做初始化
   	ov5640_data.i2c_client = client;
   	ov5640_data.pix.pixelformat = V4L2_PIX_FMT_RGB565;
   	ov5640_data.pix.width = 640;
   	ov5640_data.pix.height = 480;
   	ov5640_data.pix.field = V4L2_FIELD_NONE;
   	ov5640_data.pix.colorspace = V4L2_COLORSPACE_SRGB;
   	ov5640_data.streamcap.capability = V4L2_MODE_HIGHQUALITY |
   					   V4L2_CAP_TIMEPERFRAME;
   	ov5640_data.streamcap.capturemode = V4L2_CAP_TIMEPERFRAME;
   	ov5640_data.streamcap.timeperframe.denominator = DEFAULT_FPS;
   	ov5640_data.streamcap.timeperframe.numerator = 1;
   
   	ov5640_regulator_enable(&client->dev);
   
   	ov5640_reset(); 					//拉GPIO线，初始化外设
   
   	ov5640_power_down(0);　　　　　　　　　//拉GPIO线，ｐｏｗｅｒ＿ｄｏｗｎ
   
       /*调用II master 去发送IIC信号，从OV5640 那里获取信息.*/
   	retval = ov5640_read_reg(OV5640_CHIP_ID_HIGH_BYTE, &chip_id_high); 
   	if (retval < 0 || chip_id_high != 0x56) {
   		clk_disable_unprepare(ov5640_data.sensor_clk);
   		pr_warning("camera ov5640 is not found\n");
   		return -ENODEV;
   	}
   	retval = ov5640_read_reg(OV5640_CHIP_ID_LOW_BYTE, &chip_id_low);
   	if (retval < 0 || chip_id_low != 0x40) {
   		clk_disable_unprepare(ov5640_data.sensor_clk);
   		pr_warning("camera ov5640 is not found\n");
   		return -ENODEV;
   	}
   
   	retval = init_device();
   	if (retval < 0) {
   		clk_disable_unprepare(ov5640_data.sensor_clk);
   		pr_warning("camera ov5640 init failed\n");
   		ov5640_power_down(1);
   		return retval;
   	}
   
   	clk_disable(ov5640_data.sensor_clk);
   
   	v4l2_i2c_subdev_init(&ov5640_data.subdev, client, &ov5640_subdev_ops);
   
   	retval = v4l2_async_register_subdev(&ov5640_data.subdev);
   	if (retval < 0)
   		dev_err(&client->dev,
   					"%s--Async register failed, ret=%d\n", __func__, retval);
   
   	pr_info("camera ov5640, is found\n");
   	return retval;
   }
   ```

对于linux 学习来说,先把下面这些抽象起来

```CQL
1. 通过linux去配置时钟源是如何实现的->通过linux 去配置寄存器是如何操作的(先不纠结原理)
2. 通过linux去控制GPIO 是如何实现的.
3. linux 是如何控制主机IIC去读写从机IIC的,是需要有哪些配置?
4. linux 系统中对于操作片上外设,和片外外设有什么区别.
5. 同样,对于CSI(DVP)的接口,linux是如何初始化片上SOC的CSI 寄存器,又是如何利用master的CSI 接口去 读写 片外DVP接口的数据的．
6. I2C时候使用的IO是什么,如何使用的?
```

#### 1.3 数据传输：

其实对于`ov5640`只是一个从设备,已经没有初始化的操作了,需要把IIC的地址配对以后,就可以利用`ov5640_read_reg(OV5640_CHIP_ID_HIGH_BYTE, &chip_id_high);`的语句去进行IIC的数据传输了.这里的dev 节点应该是设备树的节点,能够从这个dev节点中获取各种配置信息.

```c
/* 传入就直接是i2c_client结构体 */
struct device *dev = &client->dev;
/* 然后把dev作为节点,获取了多个设备树的信息 类似于这样,但是由于没有和IIC强相关的操作,就不细说了*/
of_property_read_u32(dev->of_node, "mclk_source",
					(u32 *) &(ov5640_data.mclk_source));
/*把client的值,付给5640结构体*/
ov5640_data.i2c_client = client;
/*调用接口,进行IIC读,其中OV5640_CHIP_ID_HIGH_BYTE 是寄存器地址,这里是0x300A*/
retval = ov5640_read_reg(OV5640_CHIP_ID_HIGH_BYTE, &chip_id_high);



```

* ov5640_read_reg: 对传入的0x3000A做移位按照8字节发送出去,调用i2c_master_send,并且接收数据

```c
static s32 ov5640_read_reg(u16 reg, u8 *val)
{
	u8 au8RegBuf[2] = {0};
	u8 u8RdVal = 0;

	au8RegBuf[0] = reg >> 8;
	au8RegBuf[1] = reg & 0xff;

	if (2 != i2c_master_send(ov5640_data.i2c_client, au8RegBuf, 2)) {
		pr_err("%s:write reg error:reg=%x\n",
				__func__, reg);
		return -1;
	}

	if (1 != i2c_master_recv(ov5640_data.i2c_client, &u8RdVal, 1)) {
		pr_err("%s:read reg error:reg=%x,val=%x\n",
				__func__, reg, u8RdVal);
		return -1;
	}

	*val = u8RdVal;
	return u8RdVal;
}

```

* i2c_master_send: 从client中获取该使用的IIC控制器,目标地址,结合传入的数据,填充`msg`结构体,调用`i2c_transfer`进行传输.

```c
int i2c_master_send(const struct i2c_client *client, const char *buf, int count)
{
	int ret;
	struct i2c_adapter *adap = client->adapter; /*这里会确定最后用哪个adapter*/
	struct i2c_msg msg;

	msg.addr = client->addr;
	msg.flags = client->flags & I2C_M_TEN;
	msg.len = count;
	msg.buf = (char *)buf;

	ret = i2c_transfer(adap, &msg, 1);

	/*
	 * If everything went ok (i.e. 1 msg transmitted), return #bytes
	 * transmitted, else error code.
	 */
	return (ret == 1) ? count : ret;
}
```

* i2c_master_recv 的操作是类似的.

```c
int i2c_master_recv(const struct i2c_client *client, char *buf, int count)
{
	struct i2c_adapter *adap = client->adapter;
	struct i2c_msg msg;
	int ret;

	msg.addr = client->addr;
	msg.flags = client->flags & I2C_M_TEN;
	msg.flags |= I2C_M_RD;
	msg.len = count;
	msg.buf = buf;

	ret = i2c_transfer(adap, &msg, 1);

	/*
	 * If everything went ok (i.e. 1 msg received), return #bytes received,
	 * else error code.
	 */
	return (ret == 1) ? count : ret;
}
EXPORT_SYMBOL(i2c_master_recv);
```

* 所以和硬件相关的关键还是`i2c_client`这个结构体,它能从硬件描述的dts 信息中,解析出来驱动需要的`控制器,从机地址,发送flag`这些关键信息.

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
```

#### 1.4 待解决

对于linux 学习来说,先把下面这些抽象起来

```CQL
1. 通过linux去配置时钟源是如何实现的->通过linux 去配置寄存器是如何操作的(先不纠结原理)
2. 通过linux去控制GPIO 是如何实现的.
3. linux 是如何控制主机IIC去读写从机IIC的,是需要有哪些配置?
4. linux 系统中对于操作片上外设,和片外外设有什么区别.
5. 同样,对于CSI(DVP)的接口,linux是如何初始化片上SOC的CSI 寄存器,又是如何利用master的CSI 接口去 读写 片外DVP接口的数据的．
6. I2C时候使用的IO是什么,如何使用的?
```





```c
MMIO（Memory-Mapped I/O）是处理器与外围设备之间进行数据传输的一种常见方式，通过将设备的寄存器映射到系统的地址空间中，处理器可以使用普通的内存访问指令与设备通信。要深入了解 MMIO 及其在 Linux 中的应用，你可以从以下几个方面进行学习：

### 1. **基础知识**
   - **内存映射 I/O（MMIO）**：
     - 了解 MMIO 的基本概念，理解它如何将设备寄存器映射到处理器的地址空间中。
     - 学习 MMIO 的优缺点，以及它与端口映射 I/O（Port-Mapped I/O, PMIO）的对比。

   - **内存和 I/O 体系结构**：
     - 了解计算机体系结构中内存与 I/O 的基础知识，包括处理器如何访问内存和 I/O 设备。
     - 读写内存和 I/O 寄存器的基础知识，以及不同的总线标准（如 PCI、PCIe）如何影响这些操作。

### 2. **Linux 系统中的 MMIO**
   - **Linux 内存管理**：
     - 理解 Linux 内存管理的基本概念，包括虚拟内存、物理内存、内存映射（mmap）、页表等。
     - 研究如何使用 `ioremap`、`iounmap` 等函数将设备的物理地址映射到虚拟地址空间，以便进行 MMIO 操作。

   - **Linux 设备驱动开发**：
     - 阅读《Linux 设备驱动程序》书籍（第三版，作者 Jonathan Corbet 等），其中涵盖了 MMIO 在设备驱动开发中的应用。
     - 学习内核中提供的用于操作 MMIO 的函数，如 `readb`、`readw`、`readl`、`writeb`、`writew`、`writel` 等。

   - **内核源代码分析**：
     - 查看 Linux 内核源代码中与 MMIO 相关的部分，例如 `drivers/` 目录下的各类设备驱动程序实现。
     - 分析常见的设备驱动程序如何使用 MMIO 进行设备控制。

### 3. **相关硬件手册和标准**
   - **处理器和芯片组手册**：
     - 例如 ARM 或 x86 处理器的架构手册，了解 MMIO 是如何在硬件层面实现的。
     - 学习相关总线标准（如 PCI、PCIe）的文档，以理解设备如何通过这些总线与处理器通信。

   - **设备手册**：
     - 阅读具体设备的手册，了解其寄存器映射、操作方式，以及如何通过 MMIO 进行控制。

### 4. **高级内容**
   - **Linux 内核中的内存模型**：
     - 学习内核内存模型如何影响 MMIO 操作，特别是与 SMP（对称多处理）系统中的内存一致性相关的内容。
     - 理解内核中的屏障（Memory Barriers）以及它们如何影响 MMIO 读写操作的顺序和安全性。

   - **PCI/PCIe 等总线协议**：
     - 了解 PCI 和 PCIe 等总线协议，这些协议广泛用于连接处理器和外设，并支持 MMIO 操作。

### 5. **实践与实验**
   - **编写简单的 Linux 设备驱动程序**：
     - 编写和调试简单的 Linux 设备驱动程序，直接操作 MMIO 寄存器。你可以从简单的字符设备驱动开始，逐步实现更复杂的设备驱动程序。

   - **虚拟化与 MMIO**：
     - 学习虚拟化环境下 MMIO 的实现，如 KVM（Kernel-based Virtual Machine）如何在虚拟机中实现 MMIO。

### 推荐阅读材料：
- **Books**:
  - "Linux Device Drivers" by Jonathan Corbet, Alessandro Rubini, and Greg Kroah-Hartman.
  - "Understanding the Linux Kernel" by Daniel P. Bovet and Marco Cesati.
  
- **Online Resources**:
  - Linux Kernel Documentation (`Documentation/` in the Linux source tree).
  - ARM or Intel processor manuals, available from their respective websites.

通过上述的学习路径，你将对 MMIO 在 Linux 系统中的应用有一个全面而深入的理解，并能在实际的驱动开发和系统编程中熟练应用这些知识。
```

### 2. OV5640 的IIC 接口

### 3. OV5640中的 V4L2 框架

### 4.linux 下 OV5640驱动详解