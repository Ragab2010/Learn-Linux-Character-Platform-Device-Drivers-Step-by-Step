#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/device.h>
MODULE_LICENSE("GPL");


struct class * myclass;

static int myclass_init(void){
	printk("myclass_init called");
	myclass = class_create(THIS_MODULE , "myClass");
	return 0;
}
static void myclass_exit(void){
	printk("myclass_exit called");
	class_destroy(myclass);
}

module_init(myclass_init);
module_exit(myclass_exit);
