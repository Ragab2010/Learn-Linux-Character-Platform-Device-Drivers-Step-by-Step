#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/kdev_t.h>
#include<linux/fs.h>

MODULE_LICENSE("GPL");

int majorNumber=500;
module_param(majorNumber , int , 0644);
MODULE_PARM_DESC(majorNumber, "An integer parameter for Major number");

int minorNumber=0;
module_param(minorNumber , int , 0644);
MODULE_PARM_DESC(minorNumber, "An integer parameter for Minor number");

char *deviceName ="myCharDev";
module_param(deviceName , charp , 0644);
MODULE_PARM_DESC(deviceName, "An char pointer parameter for Device name ");

dev_t deviceNumber;
int count=1;
module_param(count, int, 0);
MODULE_PARM_DESC(deviceName, "An integer parameter for how many Minor number ");


static int static_allocate_deviceNumber_init(void){
	printk("static_allocate_deviceNumber_init");
	deviceNumber = MKDEV(majorNumber , minorNumber);
	printk("the Number is:%u\n", deviceNumber);
	printk("the Major number is:%d\n", MAJOR(deviceNumber));
	printk("the Major number is:%d\n", MINOR(deviceNumber));
	if(register_chrdev_region(deviceNumber , count , deviceName)== 0){
		printk("Device number registered\n");
	}else{
		printk("Device number registration Failed\n");
	}
	return 0;
}
static void static_allocate_deviceNumber_exit(void){
	printk("static_allocate_deviceNumber_exit");
	unregister_chrdev_region(deviceNumber , count);
}

module_init(static_allocate_deviceNumber_init);
module_exit(static_allocate_deviceNumber_exit);
