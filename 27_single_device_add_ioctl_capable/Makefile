 obj-m += single_device_add_ioctl_capable.o
#obj-m += single_device_nodes_fops.o

all:
	make -C /lib/modules/`uname -r`/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

