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

int baseNumber = 0;
module_param(baseNumber, int, 0644);
MODULE_PARM_DESC(baseNumber, "Base minor number");

char *deviceName = "msg";
module_param(deviceName, charp, 0644);
MODULE_PARM_DESC(deviceName, "Device name");

int count = 1;
module_param(count, int, 0644);
MODULE_PARM_DESC(count, "Number of devices to create");

dev_t deviceNumber;
struct class *myClass = NULL;
struct device *myDevice = NULL;

static struct cdev myCdev;
#define MAX_SIZE        1024
char kernel_buffer[MAX_SIZE];
int kernel_buffer_index;

static int myOpen(struct inode *inode, struct file *file) {
    pr_info("%s: Device opened\n", __func__);
    file->f_pos = 0;
    return 0;
}

static ssize_t myRead(struct file *file, char __user *user_buffer, size_t len, loff_t *offset) {
    ssize_t bytes_to_read;

    pr_info("%s: Read operation\n", __func__);

    // Check if offset is beyond valid data
    if (*offset >= kernel_buffer_index) {
        pr_info("%s: No more data to read\n", __func__);
        return 0; // EOF
    }

    // Limit read to available data
    bytes_to_read = min_t(size_t, len, kernel_buffer_index - *offset);
    if (bytes_to_read == 0) {
        pr_info("%s: No data available to read\n", __func__);
        return 0;
    }

    // Copy data to user space
    if (copy_to_user(user_buffer, kernel_buffer + *offset, bytes_to_read)) {
        pr_err("%s: Failed to copy data to user\n", __func__);
        return -EFAULT;
    }

    *offset += bytes_to_read;

    pr_info("%s: Read %zd bytes, offset now %lld\n", __func__, bytes_to_read, *offset);
    return bytes_to_read;
}

static ssize_t myWrite(struct file *file, const char __user *user_buffer, size_t len, loff_t *offset) {
    ssize_t bytes_to_write;

    pr_info("%s: Write operation\n", __func__);

    // Check if write would exceed buffer size
    if (*offset >= MAX_SIZE) {
        pr_err("%s: Offset beyond buffer\n", __func__);
        return -ENOSPC;
    }

    // Limit write to remaining buffer space
    bytes_to_write = min_t(size_t, len, MAX_SIZE - *offset);
    if (bytes_to_write == 0) {
        pr_err("%s: No space left in buffer\n", __func__);
        return -ENOSPC;
    }

    // Copy data from user space
    if (copy_from_user(kernel_buffer + *offset, user_buffer, bytes_to_write)) {
        pr_err("%s: Failed to copy data from user\n", __func__);
        return -EFAULT;
    }

    *offset += bytes_to_write;

    // Update kernel_buffer_index if write extends valid data
    if (*offset > kernel_buffer_index) {
        kernel_buffer_index = *offset;
    }

    pr_info("%s: Wrote %zd bytes, offset now %lld\n", __func__, bytes_to_write, *offset);
    pr_info("%s: kernel_buffer content: %.*s\n", __func__, kernel_buffer_index, kernel_buffer);
    return bytes_to_write;
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

static int __init cdev_init_example_init(void) {
    int ret;

    pr_info("Initializing character device using cdev_init()\n");

    ret = alloc_chrdev_region(&deviceNumber, baseNumber, count, deviceName);
    if (ret < 0) {
        pr_err("Failed to allocate device number\n");
        return ret;
    }

    cdev_init(&myCdev, &myF_ops);
    myCdev.owner = THIS_MODULE;

    ret = cdev_add(&myCdev, deviceNumber, count);
    if (ret < 0) {
        pr_err("Failed to add cdev\n");
        unregister_chrdev_region(deviceNumber, count);
        return ret;
    }

    myClass = class_create(THIS_MODULE, "myClass");
    if (IS_ERR(myClass)) {
        pr_err("Failed to create class\n");
        cdev_del(&myCdev);
        unregister_chrdev_region(deviceNumber, count);
        return PTR_ERR(myClass);
    }

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
