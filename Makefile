# Specify the compiler explicitly
CC := /usr/bin/x86_64-linux-gnu-gcc-13

obj-m += idt_driver.o
ccflags-y := -fno-inline

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules CC=$(CC)

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

