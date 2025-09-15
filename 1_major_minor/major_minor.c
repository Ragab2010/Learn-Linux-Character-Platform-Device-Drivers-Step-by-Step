#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/kdev_t.h>

MODULE_LICENSE("GPL");


static int major_minor_init(void){
	dev_t majorMinorNumber = MKDEV(6,50);
	 printk("major_minor_init called");
	printk("the Number is:%u\n", majorMinorNumber);
	printk("the Major number is:%d\n", MAJOR(majorMinorNumber));
	printk("the Major number is:%d\n", MINOR(majorMinorNumber));
	return 0;
}
static void major_minor_exit(void){
	printk("major_minor_exit called");
}

module_init(major_minor_init);
module_exit(major_minor_exit);
