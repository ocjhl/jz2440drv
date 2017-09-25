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


/* �ο�:
 * drivers\mtd\devices\mtdram.c
 * drivers/mtd/devices/m25p80.c
 */


/* ����ע��һ��mtd_info
 * mtd_device_register(master, parts, nr_parts)
 *
 */


/* ����: ����ע��spi_driver
 * Ȼ��: ��spi_driver��probe�����ﹹ��ע��mtd_info
 */

static int __devinit spi_flash_probe(struct spi_device *spi)
{
    /* ����ע��һ��mtd_info
     * mtd_device_register(master, parts, nr_parts)
     *
     */
    return 0;
}

static int __devexit spi_flash_remove(struct spi_device *spi)
{
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




