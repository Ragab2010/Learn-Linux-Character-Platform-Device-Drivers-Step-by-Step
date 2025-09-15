#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

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

/*
struct cdev {
	struct kobject kobj;
	struct module *owner;
	const struct file_operations *ops;
	struct list_head list;
	dev_t dev;
	unsigned int count;
}
*/
#define MAX_SIZE        1024
struct msg_device{
    struct cdev  mycdev;
    char kernel_buffer[MAX_SIZE];
    int kernel_buffer_index;
};

struct msg_device  msg_devices[MAX_DEVICES];


static int myOpen(struct inode *inode, struct file *file) {
    struct msg_device * my_device;
    pr_info("%s: Device opened\n", __func__);
    my_device =container_of(inode->i_cdev, struct msg_device, mycdev);
    file->private_data=my_device;
    file->f_pos = 0;
    return 0;
}

static ssize_t myRead(struct file *file, char __user *user_buffer, size_t user_lenght, loff_t *offset) {
    ssize_t bytes_to_read;
    struct msg_device * my_device= (struct msg_device * )file->private_data;
    //here the max is the kernel_buffer_index not the max , so replace the MAX_SIZE with kernel_buffer_index with the myWrite function
    pr_info("%s: Read operation\n", __func__);

    // Check if offset is beyond valid data
    if (*offset >=  my_device->kernel_buffer_index) {
        pr_info("%s: No more data to read\n", __func__);
        return 0; // EOF
    }

    // Limit read to available data
    /*
            <---kernel_index-offset----> kernel_index        MAX
        |-----------------------------------|-----------------|

        which min (user len , <max-offset>) to be number bytes_to_read

     */
    bytes_to_read = min_t(size_t, user_lenght, my_device->kernel_buffer_index - *offset);
    if (bytes_to_read == 0) {
        pr_info("%s: No data available to read\n", __func__);
        return 0;
    }

    // Copy data to user space
    if (copy_to_user(user_buffer, my_device->kernel_buffer + *offset, bytes_to_read)) {
        pr_err("%s: Failed to copy data to user\n", __func__);
        return -EFAULT;
    }

    *offset += bytes_to_read;

    pr_info("%s: Read %zd bytes, offset now %lld\n", __func__, bytes_to_read, *offset);
    return bytes_to_read;
}

static ssize_t myWrite(struct file *file, const char __user *user_buffer, size_t user_lenght, loff_t *offset) {
    ssize_t bytes_to_write;
    struct msg_device * my_device= (struct msg_device * )file->private_data;
    pr_info("%s: Write operation\n", __func__);

    // Check if write would exceed buffer size
    if (*offset >= MAX_SIZE) {
        pr_err("%s: Offset beyond buffer\n", __func__);
        return -ENOSPC;
    }

    // Limit write to remaining buffer space
    /*
            buffer_index
        0    offset <-----max-offset----->  MAX_SIZE
        |------|------------------------------|

        which min (user_lenght , <max-offset>) to be number bytes_to_write
     */
    bytes_to_write = min_t(size_t, user_lenght, MAX_SIZE - *offset);
    if (bytes_to_write == 0) {
        pr_err("%s: No space left in buffer\n", __func__);
        return -ENOSPC;
    }

    // Copy data from user space
    if (copy_from_user(my_device->kernel_buffer + *offset, user_buffer, bytes_to_write)) {
        pr_err("%s: Failed to copy data from user\n", __func__);
        return -EFAULT;
    }

    *offset += bytes_to_write;

    // Update kernel_buffer_index if write extends valid data
    if (*offset > my_device->kernel_buffer_index) {
        my_device->kernel_buffer_index = *offset;
    }

    pr_info("%s: Wrote %zd bytes, offset now %lld\n", __func__, bytes_to_write, *offset);
    pr_info("%s: kernel_buffer content: %.*s\n", __func__, my_device->kernel_buffer_index, my_device->kernel_buffer);
    return bytes_to_write;
}

static int myRelease(struct inode *inode, struct file *file) {
    pr_info("%s: Device closed\n", __func__);
    return 0;
}
loff_t myLseek (struct file *file , loff_t offset , int whence){
    loff_t new_pos;
    struct msg_device * my_device= (struct msg_device * )file->private_data;
    pr_info("%s: Seek operation (whence=%d, offset=%lld)\n", __func__, whence, offset);

    // Calculate new position based on whence
    switch(whence){
        case SEEK_SET:
            new_pos = offset;
            break;
        case SEEK_CUR:
            new_pos = file->f_pos  + offset;
            break;
        case SEEK_END:
            new_pos = my_device->kernel_buffer_index + offset;
            break;
        default:
            pr_err("%s: Invalid whence\n", __func__);
            return -EINVAL;  // Safe: Invalid argument
    }
    // Prevent underflow: No negative positions
    if (new_pos < 0) {
        pr_err("%s: Seek to negative position\n", __func__);
        return -EINVAL;
    }
    // Prevent overflow: Clamp to MAX_SIZE (allows writes at end)
    if (new_pos > MAX_SIZE) {
        pr_info("%s: Clamping seek beyond MAX_SIZE to %d\n", __func__, MAX_SIZE);
        new_pos = MAX_SIZE;
    }
    file->f_pos = new_pos;
    pr_info("%s: New position %lld\n", __func__, new_pos);
    return new_pos;
}

static struct file_operations myfops = {
    .owner = THIS_MODULE,
    .open = myOpen,
    .read = myRead,
    .write = myWrite,
    .release = myRelease,
    .llseek = myLseek
};


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
        cdev_init(&msg_devices[device_index].mycdev , &myfops);
        msg_devices[device_index].mycdev.owner=THIS_MODULE;
        msg_devices[device_index].kernel_buffer_index=0;

        // 3. Add cdev to kernel
        returnValue = cdev_add(&msg_devices[device_index].mycdev, temp_device_number, 1);
        if (returnValue < 0) {
            pr_err("Failed to add cdev\n");
            class_destroy(myclass);
            cdev_del(&msg_devices[device_index].mycdev);
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
        cdev_del(&msg_devices[device_index].mycdev);

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
