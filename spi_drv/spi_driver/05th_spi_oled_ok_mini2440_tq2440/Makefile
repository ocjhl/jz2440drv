KERN_DIR = /work/system/linux-3.4.2

all:
	make -C $(KERN_DIR) M=`pwd` modules 

clean:
	make -C $(KERN_DIR) M=`pwd` modules clean
	rm -rf modules.order

#obj-m	+= spi_info_mini2440_tq2440.o
obj-m	+= spi_oled_drv.o
