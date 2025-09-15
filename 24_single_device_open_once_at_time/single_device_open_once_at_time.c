#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
// #include "ioctl_cmd.h"

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
char kernel_buffer[MAX_SIZE];
int kernel_buffer_index;

//atomic int value to use it to restrict  , that file open only once at time
static atomic_t device_available = ATOMIC_INIT(1);


static int myOpen(struct inode *inode, struct file *file) {
    int returnValue ;//to check return value from decrement device_available
    pr_info("%s: Device opened\n", __func__);
    /**
    * atomic_dec_and_test - decrement and test
    * @v: pointer of type atomic_t
    *
    * Atomically decrements @v by 1 and
    * returns true if the result is 0, or false for all other
    * cases.
    */
    returnValue = atomic_dec_and_test(&device_available);
    if(returnValue == 0){//false
            atomic_inc(&device_available);
            return -EBUSY;
    }
    file->f_pos = 0;
    return 0;
}

static ssize_t myRead(struct file *file, char __user *user_buffer, size_t user_lenght, loff_t *offset) {
    ssize_t bytes_to_read;
    //here the max is the kernel_buffer_index not the max , so replace the MAX_SIZE with kernel_buffer_index with the myWrite function
    pr_info("%s: Read operation\n", __func__);

    // Check if offset is beyond valid data
    if (*offset >= kernel_buffer_index) {
        pr_info("%s: No more data to read\n", __func__);
        return 0; // EOF
    }

    // Limit read to available data
    /*
            <---kernel_index-offset----> kernel_index        MAX
        |-----------------------------------|-----------------|

        which min (user len , <max-offset>) to be number bytes_to_read

     */
    bytes_to_read = min_t(size_t, user_lenght, kernel_buffer_index - *offset);
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

static ssize_t myWrite(struct file *file, const char __user *user_buffer, size_t user_lenght, loff_t *offset) {
    ssize_t bytes_to_write;

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
    // increment device_available to be 1
    atomic_inc(&device_available);
    return 0;
}
loff_t myLseek (struct file *file , loff_t offset , int whence){
    loff_t new_pos;
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
            new_pos = kernel_buffer_index + offset;
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
// long myioctl (struct file *, unsigned int cmd, unsigned long arg){
//     unsigned char ch;
//     int returnValue;
//     long size;
//     pr_info("%s: Cmd:%u\t Arg:%lu\n", __func__, cmd, arg);
//     if(_IOC_TYPE(cmd) !=MSG_MAGIC_NUMBER){
//         return  -ENOTTY;
//     }
//     if(_IOC_NR(cmd) > MSG_IOCTL_MAX_CMDS){
//         return  -ENOTTY;
//     }
//     size= _IOC_SIZE(cmd);
//     returnValue = access_ok( ( void __user * ) arg , size);
//
//     pr_info("access_ok returned:%d\n", returnValue);
// 	if (!returnValue){
// 		return -EFAULT;
//     }
// 	switch(cmd)
// 	{
// 		//Get Length of buffer
//         case MSG_IOCTL_GET_LENGTH:
//             pr_info("Get Length of buffer\n");
//             if (put_user(MAX_SIZE, (unsigned long *)arg)) {
//                 pr_err("Failed to copy data to user\n");
//                 return -EFAULT;
//             }
//             break;
// 		//clear buffer
// 		case MSG_IOCTL_CLEAR_BUFFER:
//             pr_info("clear buffer\n");
//             memset(kernel_buffer , 0 , sizeof(kernel_buffer));
//             kernel_buffer_index=0;
// 			break;
// 		//fill character
// 		case MSG_IOCTL_FILL_BUFFER:
//             pr_info("fill character\n");
//             get_user(ch , (unsigned char *)arg);
//             memset(kernel_buffer , ch , sizeof(kernel_buffer));
//             kernel_buffer_index = MAX_SIZE-1;
// 			break;
//             //address of kernel buffer
//         case MSG_GET_ADDRESS:
//             pr_info("address of kernel buffer\n");
//             //put_user(&kernel_buffer , (unsigned long long* )arg);
//             put_user((unsigned long)kernel_buffer, (unsigned long __user *)arg);
// 			break;
// 		default:
// 			pr_info("Unknown Command:%u\n", cmd);
// 			return -ENOTTY;
// 	}
// 	return 0;
// }
//
// long myioctl32bit (struct file * file, unsigned int cmd, unsigned long arg){
//         unsigned char ch;
//     int returnValue;
//     long size;
//     pr_info("%s 32bit: Cmd:%u\t Arg:%lu\n", __func__, cmd, arg);
//     if(_IOC_TYPE(cmd) !=MSG_MAGIC_NUMBER){
//         return  -ENOTTY;
//     }
//     if(_IOC_NR(cmd) > MSG_IOCTL_MAX_CMDS){
//         return  -ENOTTY;
//     }
//     size= _IOC_SIZE(cmd);
//     returnValue = access_ok( ( void __user * ) arg , size);
//
//     pr_info("access_ok returned:%d\n", returnValue);
// 	if (!returnValue){
// 		return -EFAULT;
//     }
// 	switch(cmd)
// 	{
// 		//Get Length of buffer
//         case MSG_IOCTL_GET_LENGTH:
//             pr_info("Get Length of buffer\n");
//             if (put_user(MAX_SIZE, (unsigned long *)arg)) {
//                 pr_err("Failed to copy data to user\n");
//                 return -EFAULT;
//             }
//             break;
// 		//clear buffer
// 		case MSG_IOCTL_CLEAR_BUFFER:
//             pr_info("clear buffer\n");
//             memset(kernel_buffer , 0 , sizeof(kernel_buffer));
//             kernel_buffer_index=0;
// 			break;
// 		//fill character
// 		case MSG_IOCTL_FILL_BUFFER:
//             pr_info("fill character\n");
//             get_user(ch , (unsigned char *)arg);
//             memset(kernel_buffer , ch , sizeof(kernel_buffer));
//             kernel_buffer_index = MAX_SIZE-1;
// 			break;
//             //address of kernel buffer
//         case MSG_GET_ADDRESS:
//             pr_info("address of kernel buffer\n");
//             //put_user(&kernel_buffer , (unsigned long long* )arg);
//             put_user((unsigned long)kernel_buffer, (unsigned long __user *)arg);
// 			break;
// 		default:
// 			pr_info("Unknown Command:%u\n", cmd);
// 			return -ENOTTY;
// 	}
// 	return 0;
// }

static struct file_operations myfops = {
    .owner = THIS_MODULE,
    .open = myOpen,
    .read = myRead,
    .write = myWrite,
    .release = myRelease,
    .llseek = myLseek,
//     .unlocked_ioctl=myioctl,
//     .compat_ioctl = myioctl32bit
};


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
    cdev_init(&mycdev , &myfops);
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
