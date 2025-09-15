#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/kdev_t.h>
#include<linux/fs.h>
#include<linux/device.h>
#include<linux/cdev.h>


MODULE_LICENSE("GPL");


int baseNumber=0;
module_param(baseNumber , int , 0644);
MODULE_PARM_DESC(baseNumber, "An integer parameter for base Minor number");

char *deviceName ="myCharDev";
module_param(deviceName , charp , 0644);
MODULE_PARM_DESC(deviceName, "An char pointer parameter for Device name ");

int count=1;
module_param(count , int , 0644);
MODULE_PARM_DESC(count, " An integer parameter for count number");

dev_t deviceNumber;

struct class * myClass;
struct device * myDevice;

static int allocate_device_node_init(void){
	printk("allocate_device_node_init: called");
	//create class 
	myClass = class_create(THIS_MODULE , "myClass");
	printk("class created\n");
	
	if(alloc_chrdev_region(&deviceNumber ,baseNumber, count , deviceName)== 0){
		printk("Device number registered\n");
		printk("the Major number is:%d\n", MAJOR(deviceNumber));
		myDevice = device_create(myClass , NULL , deviceNumber , NULL , deviceName );
		printk("Device Node created\n");

	}else{
		printk("Device number registration Failed\n");
	}
	return 0;
}
static void allocate_device_node_exit(void){
	printk("allocate_device_node_exit: called");
	unregister_chrdev_region(deviceNumber , count);
	device_destroy(myClass , deviceNumber);
	class_destroy(myClass);
}

module_init(allocate_device_node_init);
module_exit(allocate_device_node_exit);
