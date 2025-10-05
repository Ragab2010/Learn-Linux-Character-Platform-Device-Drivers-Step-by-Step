obj-m += my_gpio_led_driver.o

all:
	make -C /lib/modules/`uname -r`/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

