#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <sound/core.h>
#include <linux/spi/spi.h>

#include <mach/hardware.h>
#include <mach/regs-gpio.h>

/* ����ע�� spi_driver */

static int major;
static struct class *class;

static int spi_oled_dc_pin;
static struct spi_device *spi_oled_dev;
static unsigned char *ker_buf;

static void OLED_Set_DC(char val)
{
    s3c2410_gpio_setpin(spi_oled_dc_pin, val);
}

static void OLEDWriteCmd(unsigned char cmd)
{
    OLED_Set_DC(0); /* command */
    spi_write(spi_oled_dev, &cmd, 1);
    OLED_Set_DC(1); /*  */
}

static void OLEDWriteDat(unsigned char dat)
{
    OLED_Set_DC(1); /* data */
    spi_write(spi_oled_dev, &dat, 1);
    OLED_Set_DC(1); /*  */
}

static void OLEDSetPageAddrMode(void)
{
    OLEDWriteCmd(0x20);
    OLEDWriteCmd(0x02);
}

static void OLEDSetPos(int page, int col)
{
    OLEDWriteCmd(0xB0 + page); /* page address */

    OLEDWriteCmd(col & 0xf);   /* Lower Column Start Address */
    OLEDWriteCmd(0x10 + (col >> 4));   /* Lower Higher Start Address */
}


static void OLEDClear(void)
{
    int page, i;
    for (page = 0; page < 8; page ++)
    {
        OLEDSetPos(page, 0);
        for (i = 0; i < 128; i++)
            OLEDWriteDat(0);
    }
}

void OLEDClearPage(int page)
{
    int i;
    OLEDSetPos(page, 0);
    for (i = 0; i < 128; i++)
        OLEDWriteDat(0);    
}

void OLEDInit(void)
{
    /* ��OLED�������Գ�ʼ�� */
    OLEDWriteCmd(0xAE); /*display off*/ 
    OLEDWriteCmd(0x00); /*set lower column address*/ 
    OLEDWriteCmd(0x10); /*set higher column address*/ 
    OLEDWriteCmd(0x40); /*set display start line*/ 
    OLEDWriteCmd(0xB0); /*set page address*/ 
    OLEDWriteCmd(0x81); /*contract control*/ 
    OLEDWriteCmd(0x66); /*128*/ 
    OLEDWriteCmd(0xA1); /*set segment remap*/ 
    OLEDWriteCmd(0xA6); /*normal / reverse*/ 
    OLEDWriteCmd(0xA8); /*multiplex ratio*/ 
    OLEDWriteCmd(0x3F); /*duty = 1/64*/ 
    OLEDWriteCmd(0xC8); /*Com scan direction*/ 
    OLEDWriteCmd(0xD3); /*set display offset*/ 
    OLEDWriteCmd(0x00); 
    OLEDWriteCmd(0xD5); /*set osc division*/ 
    OLEDWriteCmd(0x80); 
    OLEDWriteCmd(0xD9); /*set pre-charge period*/ 
    OLEDWriteCmd(0x1f); 
    OLEDWriteCmd(0xDA); /*set COM pins*/ 
    OLEDWriteCmd(0x12); 
    OLEDWriteCmd(0xdb); /*set vcomh*/ 
    OLEDWriteCmd(0x30); 
    OLEDWriteCmd(0x8d); /*set charge pump enable*/ 
    OLEDWriteCmd(0x14); 

    OLEDSetPageAddrMode();

    OLEDClear();
    
    OLEDWriteCmd(0xAF); /*display ON*/    
}


#define OLED_CMD_INIT       0x100001
#define OLED_CMD_CLEAR_ALL  0x100002
#define OLED_CMD_CLEAR_PAGE 0x100003
#define OLED_CMD_SET_POS    0x100004

static long oled_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int page;
    int col;
    
    switch (cmd)
    {
        case OLED_CMD_INIT:
        {
            OLEDInit();
            break;
        }
        case OLED_CMD_CLEAR_ALL:
        {
            OLEDClear();
            break;
        }
        case OLED_CMD_CLEAR_PAGE:
        {
            page = arg;
            OLEDClearPage(page);
            break;
        }
        case OLED_CMD_SET_POS:
        {
            page = arg & 0xff;
            col  = (arg >> 8) & 0xff;
            OLEDSetPos(page, col);
            break;
        }
    }
    return 0;
}

static ssize_t oled_write(struct file *file,
				      const char __user *buf,
				      size_t count, loff_t *ppos)
{
    if (count > 4096)
        return -EINVAL;
    copy_from_user(ker_buf, buf, count);
    spi_write(spi_oled_dev, ker_buf, count);
    return 0;
}


static struct file_operations oled_ops = {
	.owner            = THIS_MODULE,
	.unlocked_ioctl   = oled_ioctl,
	.write            = oled_write,
};

static int __devinit spi_oled_probe(struct spi_device *spi)
{
    spi_oled_dev = spi;
    spi_oled_dc_pin = (int)spi->dev.platform_data;
    s3c2410_gpio_cfgpin(spi_oled_dc_pin, S3C2410_GPIO_OUTPUT);
    s3c2410_gpio_cfgpin(spi->chip_select, S3C2410_GPIO_OUTPUT);

    ker_buf = kmalloc(4096, GFP_KERNEL);
    
    /* ע��һ�� file_operations */
    major = register_chrdev(0, "oled", &oled_ops);

	class = class_create(THIS_MODULE, "oled");

	/* Ϊ����mdev������Щ��Ϣ�������豸�ڵ� */
	device_create(class, NULL, MKDEV(major, 0), NULL, "oled"); /* /dev/oled */
    
    
    return 0;
}

static int __devexit spi_oled_remove(struct spi_device *spi)
{

	device_destroy(class, MKDEV(major, 0));
	class_destroy(class);
	unregister_chrdev(major, "oled");

    kfree(ker_buf);
    
	return 0;
}


static struct spi_driver spi_oled_drv = {
	.driver = {
		.name	= "oled",
		.owner	= THIS_MODULE,
	},
	.probe		= spi_oled_probe,
	.remove		= __devexit_p(spi_oled_remove),
};

static int spi_oled_init(void)
{
    return spi_register_driver(&spi_oled_drv);
}

static void spi_oled_exit(void)
{
    spi_unregister_driver(&spi_oled_drv);
}

module_init(spi_oled_init);
module_exit(spi_oled_exit);
MODULE_DESCRIPTION("OLED SPI Driver");
MODULE_AUTHOR("weidongshan@qq.com,www.100ask.net");
MODULE_LICENSE("GPL");


