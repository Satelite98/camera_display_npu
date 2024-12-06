#include <linux/types.h> 
#include <linux/kernel.h> 
#include <linux/delay.h> 
#include <linux/ide.h> 
#include <linux/init.h> 
#include <linux/module.h> 
#include <linux/errno.h> 
#include <linux/gpio.h> 
#include <linux/cdev.h> 
#include <linux/device.h> 
#include <linux/of.h> 
#include <linux/of_address.h> 
#include <asm/mach/map.h> 
#include <asm/uaccess.h> 
#include <asm/io.h> 


struct dtsled_dev{
    dev_t devid;
    struct cdev cdev;
    struct class * class;
    struct device* device;
    int major;
    int minor;
    struct device_node * nd;
};

struct dtsled_dev dtsled_point;

#define DTSLED_CNT      1             /* 设备号个数 */ 
#define DTSLED_NAME      "dtsled"     /* 名字     */ 
#define LEDOFF            0           /* 关灯     */ 
#define LEDON              1           /* 开灯     */ 

static void __iomem *IMX6U_CCM_CCGR1; 
static void __iomem *SW_MUX_GPIO1_IO03; 
static void __iomem *SW_PAD_GPIO1_IO03; 
static void __iomem *GPIO1_DR; 
static void __iomem *GPIO1_GDIR; 

int led_open (struct inode * pnode, struct file * pfile)
{

    
}

ssize_t led_read (struct file * pfile, char __user * puser, size_t size, loff_t* ploff)
{


}

ssize_t led_write (struct file * pfile , const char __user * puser, size_t size, loff_t * ploff)
{


}
int led_relase (struct inode * pnode, struct file *pfile)
{


}

static struct file_operations  dts_led_fops= {
    .owner = THIS_MODULE,
    .open = led_open,
    .read = led_read,
    .write = led_write,
    .release = led_relase,
}; 

static int __init led_init(void) 
{   
    u32 val = 0; 
    int ret; 
    u32 regdata[14]; 
    const char *str; 
    struct property *proper; 
    /* find by node name */
    dtsled_point.nd = of_find_node_by_path("/alphaled");
    if (dtsled_point.nd == NULL){
        printk("alphaled node can not founed \r\n");
        return (-EINVAL);
    }else {
        printk("alphaled node has been founed \r\n");
    }

    /* get  compatible from node   */
    proper  = of_find_property(dtsled_point.nd, "compatible", NULL);
    if(proper == NULL){
        printk("compatible property find failed \r\n");
    }else {
        printk("compatible property = %s\r\n ",(char *)proper->value);
    }

    /* get status from node */
    ret  = of_property_read_string(dtsled_point.nd, "status", &str);
    if(ret <  0){
        printk("status read failed \r\n");
    }else {
        printk("status = %s\r\n ",str);
    }

    /* read data type */
    ret = of_property_read_u32_array(dtsled_point.nd, "reg", regdata, 10);
    if(ret <  0){
        printk("read reg failed \r\n");
    }else {
        u8 i=0;
        printk("read reg data: \r\n ");
        for( i = 0 ;i<10;i++)
        {
             printk("%#X ", regdata[i]); 
        }
          printk("\r\n "); 
    }

    /* reg remap */

#if 1 
    IMX6U_CCM_CCGR1 = ioremap(regdata[0], regdata[1]); 
    SW_MUX_GPIO1_IO03 = ioremap(regdata[2], regdata[3]); 
    SW_PAD_GPIO1_IO03 = ioremap(regdata[4], regdata[5]); 
    GPIO1_DR = ioremap(regdata[6], regdata[7]); 
    GPIO1_GDIR = ioremap(regdata[8], regdata[9]); 
#else 
    IMX6U_CCM_CCGR1 = of_iomap(dtsled.nd, 0); 
    SW_MUX_GPIO1_IO03 = of_iomap(dtsled.nd, 1); 
    SW_PAD_GPIO1_IO03 = of_iomap(dtsled.nd, 2); 
    GPIO1_DR = of_iomap(dtsled.nd, 3); 
    GPIO1_GDIR = of_iomap(dtsled.nd, 4); 
#endif 

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
  return 0;
}

static int __exit led_exit(void) 
{


}



module_init(led_init); 
module_exit(led_exit); 
MODULE_LICENSE("GPL"); 
MODULE_AUTHOR("zuozhongkai"); 