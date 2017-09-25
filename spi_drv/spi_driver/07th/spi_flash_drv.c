#include <linux/init.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <sound/core.h>
#include <linux/spi/spi.h>
#include <asm/uaccess.h>

#include <mach/hardware.h>
#include <mach/regs-gpio.h>

#include <linux/gpio.h>
#include <plat/gpio-cfg.h>

#include <linux/mtd/cfi.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>


/* 参考:
 * drivers\mtd\devices\mtdram.c
 * drivers/mtd/devices/m25p80.c
 */


/* 构造注册一个mtd_info
 * mtd_device_register(master, parts, nr_parts)
 *
 */


/* 首先: 构造注册spi_driver
 * 然后: 在spi_driver的probe函数里构造注册mtd_info
 */

static struct mtd_info spi_flash_dev;

static int __devinit spi_flash_probe(struct spi_device *spi)
{
    /* 构造注册一个mtd_info
     * mtd_device_register(master, parts, nr_parts)
     *
     */

	/* Setup the MTD structure */
	spi_flash_dev.name = "100ask_spi_flash";
	spi_flash_dev.type = MTD_NORFLASH;
	spi_flash_dev.flags = MTD_CAP_NORFLASH;
	spi_flash_dev.size = 0x200000;  /* 2M */
	spi_flash_dev.writesize = 1;
	spi_flash_dev.writebufsize = 4096; /* 没有用到 */
	spi_flash_dev.erasesize = 4096;  /* 擦除的最小单位 */

	spi_flash_dev.owner = THIS_MODULE;
	spi_flash_dev._erase = ram_erase;
	spi_flash_dev._read = ram_read;
	spi_flash_dev._write = ram_write;

    mtd_device_register(&spi_flash_dev, NULL, 0);
     
    return 0;
}

static int __devexit spi_flash_remove(struct spi_device *spi)
{
    mtd_device_unregister(&spi_flash_dev);
    return 0;
}

static struct spi_driver spi_flash_drv = {
	.driver = {
		.name	= "100ask_spi_flash",
		.owner	= THIS_MODULE,
	},
	.probe		= spi_flash_probe,
	.remove		= __devexit_p(spi_flash_remove),
};

static int spi_flash_init(void)
{
    return spi_register_driver(&spi_flash_drv);
}

static void spi_flash_exit(void)
{
    spi_unregister_driver(&spi_flash_drv);
}

module_init(spi_flash_init);
module_exit(spi_flash_exit);
MODULE_DESCRIPTION("Flash SPI Driver");
MODULE_AUTHOR("weidongshan@qq.com,www.100ask.net");
MODULE_LICENSE("GPL");




