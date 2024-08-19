# camera 的硬件描述

[toc]

## 1. imax6ull 的CSI 接口

此章节可参见IMX6ULL RM。Imax6ull 只有一个camera 的接口，为CSI接口。这种CSI 接口能够支持两种传输协议，一种就是传统的VSYNC 、HSYNC、data 线的传输协议，另外便是支持CCIR656 video 格式传输协议。



## 2.OV5640 支持的硬件接口

1.8V 情况下的pin脚连接如图所示，其中`PWDN`表示的是`power down`引脚，和IP 的断掉时序有关。这类的`power down`更多的是指低功耗状态，通讯传输结束之后就可以进入power down的状态，如果是需要进入电源关闭状态，可能是power off。

<img src=".\camera_hardware.assets\image-20240811110237830.png" width=70%>

* 不同图像传输时用的不同数据线：The video port of OV5640 has 10-bit, D[9:0]. For 10-bit RGB raw output, please use D[9:0]. For 8-bit YCbCr or 8-bit RGB raw or 8-bit RGB 565 output, please use D[9:2].
* 能够利用I2C 的协议进行通讯，并且读写寄存器的地址不同：`The SCCB bus of OV5640 camera module could be shared with other I2C device. When OV5640 is working, the read/write operation is separated by device address. The device address of OV5640 is 0x78 for write and 0x79 for read. I2C read/write to address other than the 2 address above will not affect SCCB registers of OV5640.`SCCB：serial camera control bus
* 两个是时钟的不同作用，一个是对于模块的输入时钟XLCK，一个是对于模块的输出时钟PCLK，更推荐利用PLCK 作为像素数据的输出CLK.



### 2.1 OV5640 支持的两种传输协议

#### 2.1.1 SCCB 协议

SCCB 协议是一种类IIC 协议的规定，由此先复习下IIC协议：

##### IIC协议

* 物理层面上：双线通讯，SCL-时钟线，SDA-数据线，全双工通讯，数据在SCL 为高电平时采样，都需要上拉。

* 数据层面上：SDA 0->1 表示起始信号，SDA 1->0 是结束信号。其中每次是数据传输为多个SCL CLK周期，**数据段为８bit有效数据 + 1bit 从机应答**

  <img src=".\camera_hardware.assets\image-20240813230448886.png" width=70%>

* 协议层面上-主从机的读写：

  从整个协议的层面上来说，IIC的协议层包括：==起始信号+从机地址+读/写bit + 数据段【数据+应答】+ 停止信号。==

  * 主机写数据到从机：

  <img src=".\camera_hardware.assets\image-20240814000722862.png">

  * 主机到从机读取数据：

  <img src=".\camera_hardware.assets\image-20240814000751264.png">
---

##### SCCB协议

* 物理层面上：双线通讯，SCL-时钟线，SDA-数据线，全双工通讯，数据在SCL 为高电平时采样，都需要上拉。

* 数据层面上：SDA 0->1 表示起始信号，SDA 1->0 是结束信号。其中每次是数据传输为多个SCL CLK周期，**数据段为８bit有效数据 + 1bit 从机应答**

* 协议层面上,其实大体上也符合IIC 协议,只不过对于这个协议,对**第一个DATA,固化为了寄存器**地址,并且每次主机发送一个数据信号之后,从机需要给出一个应答,但是主机不会check.由此SSCB 读写的指令分别如下:

  * SCCB  写: ==起始信号+从机地址(包含读写控制bit)+寄存器地址+data+结束信号==

  <img src=".\camera_hardware.assets\1536533-20190812094735968-982810165.png" width=70%>

  * SCCB  读: ==起始信号+从机地址(包含读写控制bit)+寄存器地址+data1+结束信号==

  <img src=".\camera_hardware.assets\1536533-20190812094801240-2109883530.png" width = 70%>

  * 和IIC 读的区别

    <img src=".\camera_hardware.assets\f1218001876f86aefcfa218ab120040a.png"  width=80%>

---

#### 2.1.2 DVP 协议

​			DVP 协议没有找到明确的协议说明,从CSDN的获取到的信息如下:

​			物理层面上: 由帧同步信号(VSYNC),行同步信号(HSYNC),CLK 信号线,数据线(DATA 9-0)构成.在HREF 有效时,一个PCLK 传递一个data[9-0]数据线上的bit.

​			总通信流程为:Vsync 的信号线拉起,HSYNC信号线拉起,HREF 信号线拉起,PCLK 带动数据传输.,一行传输完成后,HREF拉低,HSYNC拉低,等待下次HYSNC/EF  信号拉高后进行传输.

<img src=".\camera_hardware.assets\image-20240815235358931.png">

<img src=".\camera_hardware.assets\image-20240815235435620.png">

### 2.2 OV5640 的硬件功能

在这个细分领域下面，有机会可以看下具体的书籍，但是还是先把握Linux内核吧。

* 自动曝光和自动增益补偿功能
  * 平均区域自动曝光：通过寄存器设值一个average  window ,AEC 算法就会通过这个区域内的平均值进行曝光。
  * LAEC：在亮度过大情况下打开， night mode:在亮度过暗情况下打开。
* 支持手动设置曝光模式，通过设置0x3503寄存器，并且曝光时间在0x3500~0x3502 寄存器中设置，并且为line 对齐的模式。曝光的最大值在 0x380E/0x0380F 中设置。
* 黑色等级校准
*  light flickering frequency selection（白色闪光频率选择）
*  digital gain - 数字增益
*  strobe flash and frame exposure  频闪 和帧曝光
*  frame exposure (FREX) mode -支持整帧曝光模式
* image sensor processor digital functions-内有ISP处理器。

#### 2.2.1 上电初始化流程

* 上电1ms之后可以读写寄存器，RESETB 制高20MS以后可以利用IIC 控制寄存器

#### 2.2.2  摄像控制流程







##### 支持的功能

*  mirror and flip（镜像和翻转）
*  image windowing：控制图像采样窗口 REG 0x3800-0x3813
* test pattern: 提供测试图片输出 REG 0x503D
* AEC/AGC algorithms : 自动曝光和补偿算法 0x3500-0x350D 0x3A0F-0x3A1F
* black level calibration：黑色校准  0x4000-0x4009
* light frequency selection：闪光频率选择 0x3C01-0x3C0C
* strobe flash and frame exposure：单个曝光和整帧曝光
  * strober flash：
    * 通过SCCB 控制strobe 请求信号 和 模式
    * 只有在strobe 信号线为高的时候，曝光才有效，数据才有效。
    * xenon 单次曝光、LED1& LED2 脉冲有效曝光、LED3 长有效脉冲曝光。
  * frame exposure 帧曝光模式：在帧曝光模式下，整个帧的图片在一个时间曝光，之后快门关闭，然后数据被读出，数据全部读出之后，再开始下一次的打开快门曝光。OV5640支持两种帧曝光方式
    * FREX pin 脚控制
    * SCCB控制： REG  0x3B00-0x3B0C
* 有ISP 处理器，支持多种ISP 图像算法。【这部分内容应该是重点也是难点，但是此次重点不在此】

#### 2.2.3  数据读取流程

#####  图像输出6种timming



#### 2.2.4  状态机控制

* reset : RESETB PIN  接低或者 REG 0x3008[7]  set high
* hardware standby: PWDN pin tied high
* software standby: 