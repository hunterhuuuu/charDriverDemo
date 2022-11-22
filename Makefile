KERN_DIR ?= /usr/src/linux-headers-$(shell uname -r)/	#内核源码目录, 此处使用当前系统的
PWD := $(shell pwd)
 
obj-m := charDriverDemo.o
 
all:
	make -C $(KERN_DIR) M=$(PWD) modules
 
clean:
	 make -C $(KERN_DIR) M=$(PWD) clean
