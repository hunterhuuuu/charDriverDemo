#include <linux/init.h>   	//module_init(),module_exit()
#include <linux/module.h> 	//MODULE_LICENSE(), MODULE_AUTHOR()
#include <linux/cdev.h>   	//cdev_alloc()、cdev_init()、cdev_add()、cdev_del()
#include <linux/uaccess.h>	//copy_to_user()、copy_from_user()、kfree()
#include <linux/slab.h>
#include <linux/fs.h>
#define BASE_MINOR  	0		//次设备号从0开始计数
#define DEV_COUNT   	1		//1的含义是：本驱动能驱动1个同类设备
#define DEV_NAME    	"chrDevDemo"	//字符设备名称，到时候生成的节点名就是/dev/chrDevDemo
#define BUF_LEN     	128		//测试数据缓冲区大小
#define IOCTL_CMD0	(0U)
#define IOCTL_CMD1	(1U)

dev_t devNo;				//设备号 由主设备号12bit + 20bit次设备组成
struct cdev *pCDev;			//字符设备
struct class *pClass;
struct device *pDevice;

static int dataLen = BUF_LEN;		//数据缓冲区长度
static char kernelBuf[BUF_LEN] = "Hello Char Device Driver";	//用来作为数据缓冲区

//对应系统调用open
static int CharDevOpen(struct inode *inode, struct file *filp)
{
    printk(KERN_INFO "[%s:%s-%d] CharDevOpen\r\n", __FILE__, __func__, __LINE__);
    return 0;
}

//对应系统调用read
static ssize_t CharDevRead (struct file *file, char __user *buf,
            size_t cnt, loff_t *offt)
{
    int ret = 0;
    int len = (cnt>dataLen)?dataLen:cnt;
    ret = copy_to_user(buf, kernelBuf, len);
    if (!ret) {
		ret = len;
        printk(KERN_INFO "kernel copy_to_user: %.128s\r\n", kernelBuf);
    } else {
        printk(KERN_INFO "kernel copy_to_user fail, %d\r\n", ret);
    }
 
    return ret;
}

//对应系统调用write
static ssize_t CharDevWrite (struct file *file, const char __user *buf,
            size_t cnt, loff_t *offt)
{
    int ret = 0;
	int len = (cnt>dataLen)?dataLen:cnt;
    ret = copy_from_user(kernelBuf, buf, len);
    if (!ret) {
        dataLen = len;
		ret = len;
        printk(KERN_INFO "kernel copy_from_user: %.128s\r\n", kernelBuf);
    } else {
        ret = 0;
        printk(KERN_INFO "kernel copy_from_user fail, %d\r\n", ret);
    }
 
    return ret;
}

//对应系统调用close
static int CharDevRelease(struct inode *inode, struct file *filp)
{
    printk(KERN_INFO "[%s:%s-%d] CharDevRelease\r\n", __FILE__, __func__, __LINE__);
    return 0;
}

//对应系统调用ioctl
static long int CharDevIoctl(struct file *filp, unsigned int cmd, long unsigned int arg)
{
    switch (cmd)
    {
    	case IOCTL_CMD0:
    		printk(KERN_ERR "IOCTL_CMD0 called\n");
    		break;
    	case IOCTL_CMD1:
    		printk(KERN_DEBUG "IOCTL_CMD1 called\n");
    		break;
    	default:
    		printk(KERN_INFO "unknown ioctl command\n");
    		break;
    }
    return 0;
}


static struct file_operations fops = {	//操作集
    .owner      	= THIS_MODULE,
    .open       	= CharDevOpen,
    .read       	= CharDevRead,
    .write      	= CharDevWrite,
    .release    	= CharDevRelease,
    .unlocked_ioctl	= CharDevIoctl,
};

static int __init CharDevInit(void)		//入口函数, 设备初始化
{
    int ret;
    //1、申请设备号
#if 0 //动态申请
    ret = alloc_chrdev_region(&devNo, BASE_MINOR, DEV_COUNT, DEV_NAME);
    if (0 > ret) {
        printk(KERN_ERR "[%s:%s-%d] alloc_chrdev_region fail\r\n", __FILE__, __func__, __LINE__);
        goto err1;
    }
#else //静态申请--也就是指定主设备号
#define CHRDEV_MAJOR 210
    devNo = MKDEV(CHRDEV_MAJOR, BASE_MINOR);
    ret = register_chrdev_region(devNo, DEV_COUNT, DEV_NAME);
    if (0 > ret) {
        printk(KERN_ERR "[%s:%s-%d] register_chrdev_region fail\r\n", __FILE__, __func__, __LINE__);
        goto err1;
    }
#endif

    //2、申请cdev
    pCDev = cdev_alloc();
    if (NULL == pCDev) {        
        printk(KERN_ERR "[%s:%s-%d] cdev_alloc fail\r\n", __FILE__, __func__, __LINE__);
        ret = (-ENOMEM);
        goto err2;
    }

    //3、初始化cdev
    cdev_init(pCDev, &fops);
    pCDev->owner = THIS_MODULE;

    //4、添加cdev
    ret = cdev_add(pCDev, devNo, DEV_COUNT);
    if (0 > ret) {
        printk(KERN_ERR "[%s:%s-%d] cdev_alloc fail\r\n", __FILE__, __func__, __LINE__);
        goto err3;
    }

    //5、创建class
    pClass = class_create(THIS_MODULE, DEV_NAME);
    if (IS_ERR(pDevice)) {
        printk(KERN_ERR "[%s:%s-%d] class_create fail\r\n", __FILE__, __func__, __LINE__);
        ret = PTR_ERR(pClass);
        goto err4;
    }

    //6、创建device
    pDevice = device_create(pClass, NULL, devNo, NULL, DEV_NAME);
    if (IS_ERR(pDevice)) {
        printk(KERN_ERR "[%s:%s-%d] device_create fail\r\n", __FILE__, __func__, __LINE__);
        ret = PTR_ERR(pDevice);
        goto err5;
    }

    printk(KERN_INFO "[%s:%s-%d] Init success dev:%s, MAJOR:%d, MINOR:%d.\r\n", __FILE__, __func__, __LINE__, 
        DEV_NAME, MAJOR(devNo), MINOR(devNo));

    return 0;

err5:
    class_destroy(pClass);
err4:
    cdev_del(pCDev);
err3:
    kfree(pCDev);
err2:
    unregister_chrdev_region(devNo, DEV_COUNT);
err1:
    return ret;
}

//出口函数 释放资源，注销设备
static void __exit CharDevUninit(void)
{    
    device_destroy(pClass, devNo);
    class_destroy(pClass);
    cdev_del(pCDev);
    unregister_chrdev_region(devNo, DEV_COUNT);
    printk(KERN_INFO "[\r\n\r\n%s:%s-%d]\r\n", __FILE__, __func__, __LINE__);
}

module_init(CharDevInit);
module_exit(CharDevUninit);
MODULE_LICENSE("GPL");	//必要的协议声明
