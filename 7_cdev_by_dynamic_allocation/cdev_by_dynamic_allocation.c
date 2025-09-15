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

/*In kernel, each character device is represented using this structure.
 struct cdev {
	struct kobject kobj;
	struct module *owner;
	const struct file_operations *ops;
	struct list_head list;
	dev_t dev;
	unsigned int count;
} __randomize_layout;
*/
struct cdev * myCdev;//pointer to cdev we have to use cdev_alloc(void)


int myOpen (struct inode *, struct file *){
	pr_info("%s\n", __func__);
	return 0;
}
ssize_t myRead (struct file *, char __user *, size_t, loff_t *){
	pr_info("%s\n", __func__);
	return 0;
}
ssize_t myWrite (struct file *, const char __user *, size_t, loff_t *){
	pr_info("%s\n", __func__);
	return 0;
}
int MyRelease (struct inode *, struct file *){
	pr_info("%s\n", __func__);
	return 0;
}


//allocate file_operations myF_ops for myCdev
struct file_operations myF_ops= {
	//.owner=THIS_MODULE
	.open =myOpen,
	.read = myRead,
	.write = myWrite,
	.release= MyRelease
};


static int allocate_init(void){
	printk("allocate_init: called");
	//create class 
	myClass = class_create(THIS_MODULE , "myClass");
	printk("class created\n");
	
	if(alloc_chrdev_region(&deviceNumber ,baseNumber, count , deviceName)== 0){
		printk("Device number registered\n");
		printk("the Major number is:%d\n", MAJOR(deviceNumber));
		myDevice = device_create(myClass , NULL , deviceNumber , NULL , deviceName );
		printk("Device Node created\n");
		myCdev = cdev_alloc();
		//add the owner
		myCdev->owner =THIS_MODULE;
		//add the file_operations
		myCdev->ops=&myF_ops;
		//add dev_t deviceNumber and the count of Minor
		cdev_add(myCdev , deviceNumber , count);


	}else{
		printk("Device number registration Failed\n");
	}
	return 0;
}
static void allocate_exit(void){
	printk("allocate_device_node_exit: called");
	device_destroy(myClass , deviceNumber);
	class_destroy(myClass);
	cdev_del(myCdev);
	unregister_chrdev_region(deviceNumber , count);
}

module_init(allocate_init);
module_exit(allocate_exit);
