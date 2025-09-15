#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
MODULE_LICENSE("GPL");

char *device_name = "mydevice";
module_param(device_name , charp , 0644);
MODULE_PARM_DESC(device_name , "device_name  is string of the device name");

int basecount = 0;
module_param(basecount , int , 0644);
MODULE_PARM_DESC(basecount, "Base minor number");

int count = 1;
module_param(count, int, 0644);
MODULE_PARM_DESC(count, "Number of devices to create");

dev_t device_number;

char * class_name = "myclass";
struct class * myclass;
struct device* mydevice;
struct cdev  mycdev;

/*
 1-allocate major , minor number
 2-create class with information for device file
 3-create the device file by using class
 */

int multiple_device_init(void){
    int returnValue;
    pr_info("Initializing character device using cdev_init()\n");
    //1. allocate device_number
    returnValue = alloc_chrdev_region(&device_number ,basecount  , count , device_name);
    if(returnValue != 0 ){
        pr_err("Failed to allocate device number\n");
        return returnValue;
    }
    pr_info("Major number of Character device:%d\n" , MAJOR(device_number));
    //2. create class
    myclass=class_create(THIS_MODULE ,class_name );
    if (IS_ERR(myclass)) {
        pr_err("Failed to create class\n");
        unregister_chrdev_region(device_number, count);
        return PTR_ERR(myclass);
    }
    // 3. Create device node in /dev
    mydevice = device_create(myclass , NULL ,device_number , NULL , device_name );
    if (IS_ERR(mydevice)) {
        pr_err("Failed to create device\n");
        class_destroy(myclass);
        unregister_chrdev_region(device_number, count);
        return PTR_ERR(mydevice);
    }
    // 3. Initialize cdev (no allocation, just setup)
    cdev_init(&mycdev , NULL);
    mycdev.owner=THIS_MODULE;

    // 3. Add cdev to kernel
    returnValue = cdev_add(&mycdev, device_number, count);
    if (returnValue < 0) {
        pr_err("Failed to add cdev\n");
        class_destroy(myclass);
        cdev_del(&mycdev);
        unregister_chrdev_region(device_number, count);
        return returnValue;
    }
    pr_info("Character device initialized successfully\n");

    return 0;
}
void multiple_device_exit(void){
    pr_info("Cleaning up character device\n");
    if(mydevice){
        device_destroy(myclass, device_number);
        mydevice= NULL;
    }
    if(myclass){
        class_destroy(myclass);
        myclass= NULL;
    }
    cdev_del(&mycdev);
    unregister_chrdev_region(device_number , count);
    pr_info("Character device cleaned up successfully\n");

}


module_init(multiple_device_init);
module_exit(multiple_device_exit);
