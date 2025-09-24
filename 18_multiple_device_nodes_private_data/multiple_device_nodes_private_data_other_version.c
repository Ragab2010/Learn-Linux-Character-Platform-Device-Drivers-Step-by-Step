#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/device.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Multi-device character driver with comments");
MODULE_VERSION("1.0");

// ----------------------
// CONFIGURATION CONSTANTS
// ----------------------

#define MAX_DEVICES 5     // Maximum number of devices we support
#define MAX_SIZE    1024  // Size of each device's buffer

// ----------------------
// MODULE PARAMETERS
// ----------------------

// Device name prefix, e.g. "mydevice" -> /dev/mydevice0, /dev/mydevice1, etc.
char *device_name = "mydevice";
module_param(device_name, charp, 0644);
MODULE_PARM_DESC(device_name, "Name prefix for device nodes");

// Base minor number to start from (e.g., 0 = /dev/mydevice0)
int basecount = 0;
module_param(basecount, int, 0644);
MODULE_PARM_DESC(basecount, "Base minor number");

// Number of devices to create (user-configurable)
int count = MAX_DEVICES;
module_param(count, int, 0644);
MODULE_PARM_DESC(count, "Number of devices to create (max 5)");

// ----------------------
// DEVICE STRUCTURES
// ----------------------

static dev_t device_number;  // Encodes major and base minor number
static struct class *myclass = NULL;  // Class used for udev/sysfs integration
static struct device *mydevices[MAX_DEVICES];  // Pointers to each created device

// Per-device structure
struct msg_device {
    struct cdev mycdev;                     // Character device structure
    char kernel_buffer[MAX_SIZE];          // Fixed-size buffer for each device
    int kernel_buffer_index;               // Tracks how much valid data is in the buffer
};

// Array holding state for each of our devices
static struct msg_device msg_devices[MAX_DEVICES];

// ----------------------
// FILE OPERATIONS
// ----------------------

static int myOpen(struct inode *inode, struct file *file) {
    struct msg_device *my_device;

    // Use container_of to retrieve the struct msg_device from the embedded cdev
    my_device = container_of(inode->i_cdev, struct msg_device, mycdev);

    // Store device-specific data in file->private_data
    file->private_data = my_device;

    pr_info("%s: Device opened\n", __func__);
    return 0;
}

static int myRelease(struct inode *inode, struct file *file) {
    pr_info("%s: Device closed\n", __func__);
    return 0;
}

static ssize_t myRead(struct file *file, char __user *user_buffer, size_t user_length, loff_t *offset) {
    struct msg_device *my_device = file->private_data;
    ssize_t bytes_to_read;

    // If offset is beyond data, return EOF
    if (*offset >= my_device->kernel_buffer_index)
        return 0;

    // Limit read to available data
    bytes_to_read = min_t(size_t, user_length, my_device->kernel_buffer_index - *offset);

    // Copy data to user buffer
    if (copy_to_user(user_buffer, my_device->kernel_buffer + *offset, bytes_to_read)) {
        pr_err("%s: Failed to copy data to user\n", __func__);
        return -EFAULT;
    }

    *offset += bytes_to_read;

    pr_info("%s: Read %zd bytes from device\n", __func__, bytes_to_read);
    return bytes_to_read;
}

static ssize_t myWrite(struct file *file, const char __user *user_buffer, size_t user_length, loff_t *offset) {
    struct msg_device *my_device = file->private_data;
    ssize_t bytes_to_write;

    // Reject writes beyond MAX_SIZE
    if (*offset >= MAX_SIZE)
        return -ENOSPC;

    // Limit write to remaining space
    bytes_to_write = min_t(size_t, user_length, MAX_SIZE - *offset);

    // Copy data from user buffer into kernel buffer
    if (copy_from_user(my_device->kernel_buffer + *offset, user_buffer, bytes_to_write)) {
        pr_err("%s: Failed to copy data from user\n", __func__);
        return -EFAULT;
    }

    *offset += bytes_to_write;

    // Track max written index
    if (*offset > my_device->kernel_buffer_index)
        my_device->kernel_buffer_index = *offset;

    pr_info("%s: Wrote %zd bytes to device\n", __func__, bytes_to_write);
    return bytes_to_write;
}

static loff_t myLseek(struct file *file, loff_t offset, int whence) {
    loff_t new_pos;
    struct msg_device *my_device = file->private_data;

    switch (whence) {
        case SEEK_SET:
            new_pos = offset;
            break;
        case SEEK_CUR:
            new_pos = file->f_pos + offset;
            break;
        case SEEK_END:
            new_pos = my_device->kernel_buffer_index + offset;
            break;
        default:
            return -EINVAL;
    }

    if (new_pos < 0)
        return -EINVAL;
    if (new_pos > MAX_SIZE)
        new_pos = MAX_SIZE;

    file->f_pos = new_pos;
    return new_pos;
}

// ----------------------
// FILE OPERATIONS TABLE
// ----------------------

static const struct file_operations myfops = {
    .owner = THIS_MODULE,
    .open = myOpen,
    .release = myRelease,
    .read = myRead,
    .write = myWrite,
    .llseek = myLseek,
};

// ----------------------
// MODULE INIT FUNCTION
// ----------------------

static int __init multiple_device_init(void) {
    int ret, i;
    dev_t temp_dev;

    pr_info("Initializing multi-device character driver\n");

    // Validate user-supplied count
    if (count > MAX_DEVICES) {
        pr_err("Invalid count: %d (max allowed: %d)\n", count, MAX_DEVICES);
        return -EINVAL;
    }

    // 1. Allocate a range of device numbers
    ret = alloc_chrdev_region(&device_number, basecount, count, device_name);
    if (ret) {
        pr_err("Failed to allocate device numbers\n");
        return ret;
    }

    // 2. Create a class (visible in /sys/class)
    myclass = class_create(THIS_MODULE, "myclass");
    if (IS_ERR(myclass)) {
        unregister_chrdev_region(device_number, count);
        pr_err("Failed to create class\n");
        return PTR_ERR(myclass);
    }

    // 3. Initialize each device
    for (i = 0; i < count; i++) {
        temp_dev = MKDEV(MAJOR(device_number), basecount + i);

        // Create device node in /dev (visible to user space)
        mydevices[i] = device_create(myclass, NULL, temp_dev, NULL, "%s%d", device_name, i);
        if (IS_ERR(mydevices[i])) {
            pr_err("Failed to create device %d\n", i);
            ret = PTR_ERR(mydevices[i]);
            goto cleanup;
        }

        // Initialize the character device structure
        memset(&msg_devices[i], 0, sizeof(struct msg_device));
        cdev_init(&msg_devices[i].mycdev, &myfops);
        msg_devices[i].mycdev.owner = THIS_MODULE;

        // Add device to kernel
        ret = cdev_add(&msg_devices[i].mycdev, temp_dev, 1);
        if (ret) {
            pr_err("Failed to add cdev %d\n", i);
            device_destroy(myclass, temp_dev);
            goto cleanup;
        }
    }

    pr_info("Multi-device driver loaded successfully (count=%d)\n", count);
    return 0;

// ----------------------
// ERROR CLEANUP
// ----------------------

cleanup:
    while (--i >= 0) {
        temp_dev = MKDEV(MAJOR(device_number), basecount + i);
        device_destroy(myclass, temp_dev);
        cdev_del(&msg_devices[i].mycdev);
    }
    class_destroy(myclass);
    unregister_chrdev_region(device_number, count);
    return ret;
}

// ----------------------
// MODULE EXIT FUNCTION
// ----------------------

static void __exit multiple_device_exit(void) {
    int i;
    dev_t temp_dev;

    pr_info("Cleaning up multi-device driver\n");

    for (i = 0; i < count; i++) {
        temp_dev = MKDEV(MAJOR(device_number), basecount + i);
        device_destroy(myclass, temp_dev);          // Remove /dev node
        cdev_del(&msg_devices[i].mycdev);           // Unregister from kernel
    }

    class_destroy(myclass);                         // Remove class
    unregister_chrdev_region(device_number, count); // Free device numbers

    pr_info("Driver successfully unloaded\n");
}

// ----------------------
// MODULE HOOKS
// ----------------------

module_init(multiple_device_init);
module_exit(multiple_device_exit);
