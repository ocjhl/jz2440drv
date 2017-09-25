#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/slab.h>

#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>
#include <linux/spi/s3c24xx.h>
#include <linux/module.h>

#include <plat/regs-spi.h>


/* ¹¹Ôì×¢²áspi_master */

static struct spi_master *spi0_controller;
static struct spi_master *spi1_controller;

struct s3c_spi_info {
    unsigned int reg_base;    
};

static int s3c2440_spi_setup(struct spi_device *spi)
{
    return 0;
}

 static int s3c2440_spi_transfer(struct spi_device *spi, struct spi_message *mesg)
 {
    return 0;
 }

static struct spi_master *create_spi_master_s3c2440(int bus_num, unsigned int reg_base_phy)
{
    struct spi_master *master;
    struct s3c_spi_info *info;

    master = spi_alloc_master(NULL, sizeof(struct s3c_spi_info));
    master->bus_num = bus_num;
    master->num_chipselect = 0xffff;
	master->mode_bits = SPI_CPOL | SPI_CPHA | SPI_CS_HIGH;

    master->setup    = s3c2440_spi_setup;
    master->transfer = s3c2440_spi_transfer;

    info = spi_master_get_devdata(master);
    info->reg_base = ioremap(reg_base_phy, 0x18);

    spi_register_master(master);

    return master;
}

static void destroy_spi_master_s3c2440(struct spi_master *master)
{
    struct s3c_spi_info *info = spi_master_get_devdata(master);;
    
    spi_unregister_master(master);
    iounmap(info->reg_base);
    kfree(master);
}

static int spi_s3c2440_init(void)
{
    spi0_controller = create_spi_master_s3c2440();
    spi1_controller = create_spi_master_s3c2440();
    
    return 0;
}

static void spi_s3c2440_exit(void)
{
    destroy_spi_master_s3c2440(spi0_controller);
    destroy_spi_master_s3c2440(spi1_controller);
}

module_init(spi_s3c2440_init);
module_exit(spi_s3c2440_exit);
MODULE_DESCRIPTION("SPI Controller Driver");
MODULE_AUTHOR("weidongshan@qq.com,www.100ask.net");
MODULE_LICENSE("GPL");





