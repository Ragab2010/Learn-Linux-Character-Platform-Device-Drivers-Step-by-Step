#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple character device driver with dynamic parameters");
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

char *deviceName = "myCharDev";
module_param(deviceName, charp, 0644);
MODULE_PARM_DESC(deviceName, "Device name");

int count = 1;
module_param(count, int, 0644);
MODULE_PARM_DESC(count, "Number of devices to create");

// =======================
// Global Variables
// =======================

dev_t deviceNumber;                      // Device number (major + minor)
struct class *myClass = NULL;           // Device class
struct device *myDevice = NULL;         // Device struct
static struct cdev *myCdev = NULL;      // Character device structure

// =======================
// File Operations
// =======================

static int myOpen(struct inode *inode, struct file *file) {
    pr_info("%s: Device opened\n", __func__);
    return 0;
}

static ssize_t myRead(struct file *file, char __user *buf, size_t len, loff_t *offset) {
    pr_info("%s: Read operation\n", __func__);
    return 0;
}

static ssize_t myWrite(struct file *file, const char __user *buf, size_t len, loff_t *offset) {
    pr_info("%s: Write operation\n", __func__);
    return len;  // Returning len to indicate all bytes written
}

static int myRelease(struct inode *inode, struct file *file) {
    pr_info("%s: Device closed\n", __func__);
    return 0;
}

// File operations structure
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

static int __init allocate_init(void) {
    int ret;

    pr_info("allocate_init: Initializing character device driver\n");

    // 1. Allocate device number
    ret = alloc_chrdev_region(&deviceNumber, baseNumber, count, deviceName);
    if (ret < 0) {
        pr_err("Failed to allocate device number\n");
        return ret;
    }

    // 2. Create device class
    myClass = class_create(THIS_MODULE, "myClass");
    if (IS_ERR(myClass)) {
        pr_err("Class creation failed\n");
        unregister_chrdev_region(deviceNumber, count);
        return PTR_ERR(myClass);
    }

    // 3. Create device node in /dev
    myDevice = device_create(myClass, NULL, deviceNumber, NULL, deviceName);
    if (IS_ERR(myDevice)) {
        pr_err("Device creation failed\n");
        class_destroy(myClass);
        unregister_chrdev_region(deviceNumber, count);
        return PTR_ERR(myDevice);
    }

    // 4. Allocate and initialize cdev
    myCdev = cdev_alloc();
    if (!myCdev) {
        pr_err("cdev allocation failed\n");
        device_destroy(myClass, deviceNumber);
        class_destroy(myClass);
        unregister_chrdev_region(deviceNumber, count);
        return -ENOMEM;
    }

    myCdev->owner = THIS_MODULE;
    myCdev->ops = &myF_ops;

    // 5. Add cdev to kernel
    ret = cdev_add(myCdev, deviceNumber, count);
    if (ret < 0) {
        pr_err("cdev addition failed\n");
        kobject_put(&myCdev->kobj);  // Free allocated cdev
        device_destroy(myClass, deviceNumber);
        class_destroy(myClass);
        unregister_chrdev_region(deviceNumber, count);
        return ret;
    }

    pr_info("Character device created successfully\n");
    return 0;
}

// =======================
// Module Exit Function
// =======================

static void __exit allocate_exit(void) {
    pr_info("allocate_exit: Cleaning up character device driver\n");

    if (myCdev) {
        cdev_del(myCdev);
        myCdev = NULL;
    }

    if (myDevice) {
        device_destroy(myClass, deviceNumber);
        myDevice = NULL;
    }

    if (myClass) {
        class_destroy(myClass);
        myClass = NULL;
    }

    unregister_chrdev_region(deviceNumber, count);
    pr_info("Character device unregistered successfully\n");
}

module_init(allocate_init);
module_exit(allocate_exit);
