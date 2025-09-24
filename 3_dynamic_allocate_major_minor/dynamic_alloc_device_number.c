#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/kdev_t.h>
#include<linux/fs.h>

MODULE_LICENSE("GPL");

int majorNumber=500;
module_param(majorNumber , int , 0644);
MODULE_PARM_DESC(majorNumber, "An integer parameter for Major number");

int baseNumber=0;
module_param(baseNumber , int , 0644);
MODULE_PARM_DESC(baseNumber, "An integer parameter for base Minor number");

char *deviceName ="myCharDev";
module_param(deviceName , charp , 0644);
MODULE_PARM_DESC(deviceName, "An char pointer parameter for Device name ");

int count=1;
module_param(count , int , 0644);
MODULE_PARM_DESC(count, " An integer parameter for count number");

/*this holds the device number*/
dev_t deviceNumber;


static int dynamic_allocate_deviceNumber_init(void){
	printk("dynamic_allocate_deviceNumber_init");
	printk("the baseNumber is:%d\n", baseNumber);
	printk("the Count:%d\n", count);
	printk("the DeviceName:%s\n", deviceName);
	
	if(alloc_chrdev_region(&deviceNumber ,baseNumber, count , deviceName)== 0){
		printk("Device number registered\n");
		printk("the Major number is:%d\n", MAJOR(deviceNumber));

	}else{
		printk("Device number registration Failed\n");
	}
	return 0;
}
static void dynamic_allocate_deviceNumber_exit(void){
	printk("dynamic_allocate_deviceNumber_exit");
	unregister_chrdev_region(deviceNumber , count);
}

module_init(dynamic_allocate_deviceNumber_init);
module_exit(dynamic_allocate_deviceNumber_exit);
