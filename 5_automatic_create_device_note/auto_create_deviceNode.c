#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/kdev_t.h>
#include<linux/device.h>
#include <linux/cdev.h>

MODULE_LICENSE("GPL");

dev_t mydev1 , mydev2;

struct class * myclass;

struct device * mydev1_ptr , * mydev2_ptr;



static int auto_allocate_init(void){
	printk("auto_allocate_init called");
	mydev1=MKDEV(160,1);
	mydev2=MKDEV(160,2);
	myclass = class_create(THIS_MODULE , "myclass");
	mydev1_ptr = device_create(myclass , NULL , mydev1 , NULL , "mydev1");
	mydev2_ptr = device_create(myclass , NULL , mydev2 , NULL , "mydev2");
	return 0;
}
static void auto_allocate_exit(void){
	printk("auto_allocate_exit called");
	device_destroy(myclass , mydev1);
	device_destroy(myclass , mydev2);
	class_destroy(myclass);
}

module_init(auto_allocate_init);
module_exit(auto_allocate_exit);
