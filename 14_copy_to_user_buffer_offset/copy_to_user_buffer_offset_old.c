#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Character device using cdev_init()");
MODULE_VERSION("1.0");

/*
 struct cdev {
	struct kobject kobj;
	struct module *owner;
	const struct file_operations *ops;
	struct list_head list;
	dev_t dev;
	unsigned int count;
} __randomize_layout;
 */
// =======================
// Module Parameters
// =======================
int baseNumber = 0;
module_param(baseNumber, int, 0644);
MODULE_PARM_DESC(baseNumber, "Base minor number");

// char *deviceName = "myCharDev";
char *deviceName = "msg";
module_param(deviceName, charp, 0644);
MODULE_PARM_DESC(deviceName, "Device name");

int count = 1;
module_param(count, int, 0644);
MODULE_PARM_DESC(count, "Number of devices to create");

// =======================
// Global Variables
// =======================
dev_t deviceNumber;
struct class *myClass = NULL;
struct device *myDevice = NULL;

static struct cdev myCdev;  // Stack-allocated cdev (no need for cdev_alloc())

#define MAX_SIZE        1024
char kernel_buffer[MAX_SIZE];
int kernel_buffer_index;

// =======================
// File Operations
// =======================
static int myOpen(struct inode *inode, struct file *file) {
    pr_info("%s: Device opened\n", __func__);
    file->f_pos=0;
    return 0;
}

static ssize_t myRead(struct file *file, char __user *user_buffer, size_t len, loff_t *offset) {
    int returnValue;
    pr_info("%s: Read operation\n", __func__);
    if(kernel_buffer_index + len > MAX_SIZE){
        pr_err("%s: kernel_buffer_index:%d\t len:%lu\t MAX_SIZE:%d\n",__func__,
                kernel_buffer_index, len, MAX_SIZE);
        return -ENOSPC;
    }
    returnValue = copy_to_user(user_buffer ,kernel_buffer + *offset , len );
    pr_info("%s: Kernel_Buffer:%p\t User_Buffer:%p\n", __func__, kernel_buffer, user_buffer);
    pr_info("copy_to_user: return value :%d\n", returnValue);
    pr_info("user_buffer len :%zu\n", len);
    pr_info("user_buffer offset :%llu\n", *offset);
    pr_info("the kernel_buffer content:%s\n", kernel_buffer);
    //here se update the offset to  prevent the myRead  from read kernel_buffer from index 0
    *offset += len;
    pr_info("----------------------------\n");
    return len;
}

static ssize_t myWrite(struct file *file, const char __user *user_buffer, size_t len, loff_t *offset) {
    int returnValue;
    pr_info("%s: Write operation\n", __func__);
    if(kernel_buffer_index + len > MAX_SIZE){
        pr_err("%s: kernel_buffer_index:%d\t len:%lu\t MAX_SIZE:%d\n",__func__,
				kernel_buffer_index, len, MAX_SIZE);
		return -ENOSPC;

    }
    returnValue = copy_from_user(kernel_buffer + kernel_buffer_index ,user_buffer , len );
    pr_info("%s: Kernel_Buffer:%p\t User_Buffer:%p\n", __func__, kernel_buffer, user_buffer);
    pr_info("copy_from_user: return value :%d\n", returnValue);
    pr_info("user_buffer len :%zu\n", len);
    pr_info("user_buffer offset :%llu\n", *offset);
    pr_info("the kernel_buffer content:%s\n", kernel_buffer);
    //here se update the offset to  prevent the myWrite  to write into kernel_buffer from index 0
    kernel_buffer_index += len;
    *offset +=len;
    pr_info("----------------------------\n");
    return len;
}

static int myRelease(struct inode *inode, struct file *file) {
    pr_info("%s: Device closed\n", __func__);
    return 0;
}

static struct file_operations myF_ops = {
    .owner = THIS_MODULE,
    .open = myOpen,
    .read = myRead,
    .write = myWrite,
    .release = myRelease
};

// =======================
// Module Init Function
// =======================
static int __init cdev_init_example_init(void) {
    int ret;

    pr_info("Initializing character device using cdev_init()\n");

    // 1. Allocate device number
    ret = alloc_chrdev_region(&deviceNumber, baseNumber, count, deviceName);
    if (ret < 0) {
        pr_err("Failed to allocate device number\n");
        return ret;
    }

    // 2. Initialize cdev (no allocation, just setup)
    cdev_init(&myCdev, &myF_ops);
    myCdev.owner = THIS_MODULE;

    // 3. Add cdev to kernel
    ret = cdev_add(&myCdev, deviceNumber, count);
    if (ret < 0) {
        pr_err("Failed to add cdev\n");
        unregister_chrdev_region(deviceNumber, count);
        return ret;
    }

    // 4. Create class
    myClass = class_create(THIS_MODULE, "myClass");
    if (IS_ERR(myClass)) {
        pr_err("Failed to create class\n");
        cdev_del(&myCdev);
        unregister_chrdev_region(deviceNumber, count);
        return PTR_ERR(myClass);
    }

    // 5. Create device node in /dev
    myDevice = device_create(myClass, NULL, deviceNumber, NULL, deviceName);
    if (IS_ERR(myDevice)) {
        pr_err("Failed to create device\n");
        class_destroy(myClass);
        cdev_del(&myCdev);
        unregister_chrdev_region(deviceNumber, count);
        return PTR_ERR(myDevice);
    }

    pr_info("Character device initialized successfully\n");
    return 0;
}

// =======================
// Module Exit Function
// =======================
static void __exit cdev_init_example_exit(void) {
    pr_info("Cleaning up character device\n");

    if (myDevice) {
        device_destroy(myClass, deviceNumber);
        myDevice = NULL;
    }

    if (myClass) {
        class_destroy(myClass);
        myClass = NULL;
    }

    cdev_del(&myCdev);
    unregister_chrdev_region(deviceNumber, count);

    pr_info("Character device cleaned up successfully\n");
}

module_init(cdev_init_example_init);
module_exit(cdev_init_example_exit);
