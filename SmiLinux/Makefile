obj-m += driver.o 

driver-objs := linuxdriver.o smm.o

CC = /usr/bin/x86_64-linux-gnu-gcc-13
AS = /usr/bin/x86_64-linux-gnu-gcc-13 

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules CC=$(CC) AS=$(AS)
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

