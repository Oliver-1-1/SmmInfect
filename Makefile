obj-m += driver.o  # This is the final kernel module

driver-objs := idt_driver.o my_asm.o

# Specify the correct compiler version to avoid warning about the compiler mismatch
CC = /usr/bin/x86_64-linux-gnu-gcc-13
AS = /usr/bin/x86_64-linux-gnu-gcc-13  # Set assembler explicitly if necessary

# Default target to build the module
all:
	# Build the kernel module using the kernel's build system
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules CC=$(CC) AS=$(AS)

# Clean the build directory
clean:
	# Clean up the build artifacts
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

