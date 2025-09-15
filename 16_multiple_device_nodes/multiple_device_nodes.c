#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
MODULE_LICENSE("GPL");

#define MAX_DEVICES 5
char *device_name = "mydevice";
module_param(device_name , charp , 0644);
MODULE_PARM_DESC(device_name , "device_name  is string of the device name");

int basecount = 0;
module_param(basecount , int , 0644);
MODULE_PARM_DESC(basecount, "Base minor number");

//to allocate 5 device as minor
int count = MAX_DEVICES;
module_param(count, int, 0644);
MODULE_PARM_DESC(count, "Number of devices to create");

dev_t device_number;

char * class_name = "myclass";
struct class * myclass;
struct device* mydevice;
struct cdev  mycdev[MAX_DEVICES];

/*
 1-allocate major , minor number
 2-create class with information for device file
 3-create the device file by using class
 */

int multiple_device_init(void){
    int device_index, returnValue, major_number;
    dev_t temp_device_number;
    pr_info("Initializing character device using cdev_init()\n");
    //1. allocate device_number
    returnValue = alloc_chrdev_region(&device_number ,basecount  , count , device_name);
    if(returnValue != 0 ){
        pr_err("Failed to allocate device number\n");
        return returnValue;
    }
    pr_info("Major number of Character device:%d\n" , MAJOR(device_number));
    major_number = MAJOR(device_number);
    //2. create class
    myclass=class_create(THIS_MODULE ,class_name );
    if (IS_ERR(myclass)) {
        pr_err("Failed to create class\n");
        unregister_chrdev_region(device_number, count);
        return PTR_ERR(myclass);
    }
    /*loop from zero to MAX_DEVICES  to do
    1-make temp_device_number by using major and minor number , minor number is from zero to MAX_DEVICES.
    2-make Create device file/node for every temp_device_number
    3-Initialize cdev for every temp_device_number corresponsted to mycdev[device_index]
    4-Add cdev[device_index] to kernel
    */
    for(device_index=0 ; device_index<MAX_DEVICES ; device_index++){
        //make  device_name by using major & minor to create device file/node
        temp_device_number=MKDEV(major_number , device_index);
        // 3. Create device node in /dev
        mydevice = device_create(myclass , NULL ,temp_device_number , NULL ,"%s%d", device_name  , device_index);
        if (IS_ERR(mydevice)) {
            pr_err("Failed to create device\n");
            class_destroy(myclass);
            unregister_chrdev_region(temp_device_number, count);
            return PTR_ERR(mydevice);
        }
        // 3. Initialize cdev (no allocation, just setup)
        cdev_init(&mycdev[device_index] , NULL);
        mycdev[device_index].owner=THIS_MODULE;

        // 3. Add cdev to kernel
        returnValue = cdev_add(&mycdev[device_index], temp_device_number, 1);
        if (returnValue < 0) {
            pr_err("Failed to add cdev\n");
            class_destroy(myclass);
            cdev_del(&mycdev[device_index]);
            unregister_chrdev_region(temp_device_number, count);
            return returnValue;
        }
    }

    pr_info("Character device initialized successfully\n");

    return 0;
}
void multiple_device_exit(void){
    dev_t temp_device_number;
    int  device_index , major_number = MAJOR(device_number);
    pr_info("Cleaning up character device\n");
    for(device_index=0 ; device_index<MAX_DEVICES ; device_index++){
        temp_device_number =MKDEV(major_number , device_index);
        device_destroy(myclass, temp_device_number);
        cdev_del(&mycdev[device_index]);

    }

    if(myclass){
        class_destroy(myclass);
        myclass= NULL;
    }
    unregister_chrdev_region(device_number , count);
    pr_info("Character device cleaned up successfully\n");

}


module_init(multiple_device_init);
module_exit(multiple_device_exit);
