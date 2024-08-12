# camera 的硬件描述

## 1. imax6ull 的CSI 接口

此章节可参见IMX6ULL RM。Imax6ull 只有一个camera 的接口，为CSI接口。这种CSI 接口能够支持两种传输协议，一种就是传统的VSYNC 、HSYNC、data 线的传输协议，另外便是支持CCIR656 video 格式传输协议。



## 2.OV5640 支持的硬件接口

1.8V 情况下的pin脚连接如图所示，其中`PWDN`表示的是`power down`引脚，和IP 的断掉时序有关。这类的`power down`更多的是指低功耗状态，通讯传输结束之后就可以进入power down的状态，如果是需要进入电源关闭状态，可能是power off。

![image-20240811110237830](.\camera_hardware.assets\image-20240811110237830.png)

* 不同图像传输时用的不同数据线：The video port of OV5640 has 10-bit, D[9:0]. For 10-bit RGB raw output, please use D[9:0]. For 8-bit YCbCr or 8-bit RGB raw or 8-bit RGB 565 output, please use D[9:2].
* 能够利用I2C 的协议进行通讯，并且读写寄存器的地址不同：`The SCCB bus of OV5640 camera module could be shared with other I2C device. When OV5640 is working, the read/write operation is separated by device address. The device address of OV5640 is 0x78 for write and 0x79 for read. I2C read/write to address other than the 2 address above will not affect SCCB registers of OV5640.`SCCB：serial camera control bus
* 两个是时钟的不同作用，一个是对于模块的输入时钟XLCK，一个是对于模块的输出时钟PCLK，更推荐利用PLCK 作为像素数据的输出CLK.



### 2.1 OV5640 支持的两种传输协议

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