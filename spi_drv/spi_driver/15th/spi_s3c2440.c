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
#include <mach/regs-gpio.h>


/* ����ע��spi_master */

static struct spi_master *spi0_controller;
static struct spi_master *spi1_controller;

struct s3c_spi_info {
    int irq;
    unsigned int reg_base;    
    struct completion	 done;
    struct spi_transfer *cur_t;
    int cur_cnt;
};

static int s3c2440_spi_setup(struct spi_device *spi)
{
    struct s3c_spi_info *info;
    struct clk *clk;
    
    int cpol = 0;
    int cpha = 0;

    int div;

    info = spi_master_get_devdata(spi->master);
    clk = clk_get(NULL, "plck");
    
    /* ���ô���ģʽ : mode
     * ����Ƶ�� : max_speed_hz
     * bits_per_word : ���ù�
     */

    /* spi_mode: CPOL,CPHA���һ��ֵ,0,1,2,3
     */

    if (spi->mode & 1)
    {
        cpha = 1;
    }
    if (spi->mode & 2)
    {
        cpol = 1;
    }
    
    /* д��SPCON0,1 */
    /* [6:5] : 01, polling mode
     * [4]   : 1 = enable 
     * [3]   : 1 = master
     * [2]   : CPOL
     * [1]   : CPHA
     * [0]   : 0 = normal mode
     */
	writeb((1<<5) | (1<<4) | (1<<3) | (cpol << 2) | (cpha << 1), info->reg_base + S3C2410_SPCON);

    /*
     * Baud rate = PCLK / 2 / (Prescaler value + 1)
     * Prescaler value = PCLK/(2*Baud rate) - 1
     * Prescaler Value : 0,1,...,255
     */
	div = DIV_ROUND_UP(clk_get_rate(clk), spi->max_speed_hz * 2) - 1;

	if (div > 255)
		div = 255;

	writeb(div, info->reg_base + S3C2410_SPPRE);
        
    return 0;
}

static int s3c2440_spi_transfer(struct spi_device *spi, struct spi_message *mesg)
{
    struct spi_master *master = spi->master;
    struct s3c_spi_info *info;
	struct spi_transfer	*t = NULL;

    info = spi_master_get_devdata(master);
    
    /* 1. ѡ��оƬ */
    s3c2410_gpio_setpin(spi->chip_select, 0);  /* Ĭ��Ϊ�͵�ƽѡ�� */

    /* 2. ������ */
    
    /* 2.1 ���͵�1��spi_transfer֮ǰsetup */
    master->setup(spi);
    
    /* 2.2 ��spi_message�����ȡ��spi_transfer,ִ���� */
	list_for_each_entry (t, &mesg->transfers, transfer_list) {
	    /* �������spi_transfer */
        info->cur_t = t;
        info->cur_cnt = 0;
    	init_completion(&info->done);

        if (t->tx_buf)
        {
            /* ���� */
            writeb(((unsigned char *)t->tx_buf)[0], info->reg_base + S3C2410_SPTDAT);

            /* ���ᴥ���ж� */

            /* ���� */
            wait_for_completion(&info->done);
        }
        else if(t->rx_buf)
        {
            /* ���� */
            writeb(0xff, info->reg_base + S3C2410_SPTDAT);
            /* ���ᴥ���ж� */

            /* ���� */
            wait_for_completion(&info->done);
        }
    }
    
    
    /* 2.3 ���ѵȴ��Ľ��� */
	mesg->status = 0;
	mesg->complete(mesg->context);    

    /* 3. ȡ��Ƭѡ */
    s3c2410_gpio_setpin(spi->chip_select, 1);  /* Ĭ��Ϊ�͵�ƽѡ�� */
    
    return 0;
}

static irqreturn_t s3c2440_spi_irq(int irq, void *dev_id)
{
    struct spi_master *master = (struct spi_master *)dev_id;
    struct s3c_spi_info *info = spi_master_get_devdata(master);
    struct spi_transfer *t = info->cur_t;

    /* ���� spi_transfer */

    if (t->tx_buf) /* �Ƿ��� */
    {
        info->cur_cnt++;
        
        if (info->cur_cnt < t->len)/* û����? */
            writeb(((unsigned char *)t->tx_buf)[info->cur_cnt], info->reg_base + S3C2410_SPTDAT);        
        else
            complete(&info->done); /* ���� */
    }
    else /* ���� */
    {
        /* ��/������ */
        ((unsigned char *)t->rx_buf)[info->cur_cnt] = readb(info->reg_base + S3C2410_SPRDAT);
        info->cur_cnt++;
        
        if (info->cur_cnt < t->len)/* û����? */
            writeb(0xff, info->reg_base + S3C2410_SPTDAT);        
        else
            complete(&info->done); /* ���� */
    }
	return IRQ_HANDLED;    
}

static void s3c2440_spi_controler_init(int which)
{
	struct clk *clk = clk_get(NULL, "spi");

    /* ʹ��spi controller 0/1��ʱ�� */
    clk_enable(clk);

    /* GPIO */
    if (which == 0)
    {
        /* SPI controller 0 */
        /*
         * GPE11 SPIMISO   
         * GPE12 SPIMOSI   
         * GPE13 SPICLK    
         */
        s3c2410_gpio_cfgpin(S3C2410_GPE(11), S3C2410_GPE11_SPIMISO0);
        s3c2410_gpio_cfgpin(S3C2410_GPE(12), S3C2410_GPE12_SPIMOSI0);
        s3c2410_gpio_cfgpin(S3C2410_GPE(13), S3C2410_GPE13_SPICLK0);        

    }
    else if (which == 1)
    {
        /* SPI controller 1 */
        /*
         * GPG5 SPIMISO   
         * GPG6 SPIMOSI   
         * GPG7 SPICLK    
         */
        s3c2410_gpio_cfgpin(S3C2410_GPG(5), S3C2410_GPG5_SPIMISO1);
        s3c2410_gpio_cfgpin(S3C2410_GPG(6), S3C2410_GPG6_SPIMOSI1);
        s3c2410_gpio_cfgpin(S3C2410_GPG(7), S3C2410_GPG7_SPICLK1);        

        /* ʹ��spi controller 1��ʱ�� */
    }
}


static struct spi_master *create_spi_master_s3c2440(int bus_num, unsigned int reg_base_phy, int irq)
{
    int ret;
    struct spi_master *master;
    struct s3c_spi_info *info;

    master = spi_alloc_master(NULL, sizeof(struct s3c_spi_info));
    master->bus_num = bus_num;
    master->num_chipselect = 0xffff;
	master->mode_bits = SPI_CPOL | SPI_CPHA | SPI_CS_HIGH;

    master->setup    = s3c2440_spi_setup;
    master->transfer = s3c2440_spi_transfer;

    info = spi_master_get_devdata(master);
    info->reg_base = (unsigned int)ioremap(reg_base_phy, 0x18);
    info->irq = irq;

    /* Ӳ����ʼ�� */
    s3c2440_spi_controler_init(bus_num);

    ret = request_irq(irq, s3c2440_spi_irq, 0, "s3c2440_spi", master);

    spi_register_master(master);

    return master;
}

static void destroy_spi_master_s3c2440(struct spi_master *master)
{
    struct s3c_spi_info *info = spi_master_get_devdata(master);;
    
    spi_unregister_master(master);
    free_irq(info->irq, master);
    iounmap((void *)info->reg_base);
    kfree(master);
}

static int spi_s3c2440_init(void)
{
    spi0_controller = create_spi_master_s3c2440(0, 0x59000000, IRQ_SPI0);
    spi1_controller = create_spi_master_s3c2440(1, 0x59000020, IRQ_SPI1);
    
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





