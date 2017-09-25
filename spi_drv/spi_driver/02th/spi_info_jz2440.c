#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/io.h>

static struct spi_board_info spi_info_jz2440[] = {
	{
    	 .modalias = "oled",  /* ��Ӧ��spi_driver����Ҳ��"oled" */
    	 .max_speed_hz = 10000000,	/* max spi clock (SCK) speed in HZ */
    	 .bus_num = 1,     /* jz2440��OLED����SPI CONTROLLER 1 */
    	 .mode    = SPI_MODE_0,
    	 .chip_select   = S3C2410_GPF(1), /* oled_cs, ���ĺ�����spi_masterȷ�� */
    	 .platform_data = S3C2410_GPG(4), /* oled_dc, ����spi_driver��ʹ�� */    	 
	 },
	 {
    	 .modalias = "100ask_spi_flash",  /* ��Ӧ��spi_driver����Ҳ��"oled" */
    	 .max_speed_hz = 80000000,	/* max spi clock (SCK) speed in HZ */
    	 .bus_num = 1,     /* jz2440��OLED����SPI CONTROLLER 1 */
    	 .mode    = SPI_MODE_0,
    	 .chip_select   = S3C2410_GPG(2), /* flash_cs, ���ĺ�����spi_masterȷ�� */
	 }
}

static int spi_info_jz2440_init(void)
{
    return spi_register_board_info(spi_info_jz2440, ARRAY_SIZE(spi_info_jz2440));
}

module_init(spi_info_jz2440_init);
MODULE_DESCRIPTION("OLED SPI Driver");
MODULE_AUTHOR("weidongshan@qq.com,www.100ask.net");
MODULE_LICENSE("GPL");


