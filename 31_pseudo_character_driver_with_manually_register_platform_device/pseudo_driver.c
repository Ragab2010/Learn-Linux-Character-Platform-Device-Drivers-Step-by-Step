#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

/* 
 * Platform data (from device) is known by driver via pdev->dev.platform_data
 * Driver private data (runtime) is allocated by driver and attached via dev_set_drvdata
 */

/* Platform data structure from device */
struct pseudo_platform_data {
    int buffer_size;
    const char *device_name;
};

/* Driver private data structure */
struct pseudo_driver_data {
    char *buffer;   // runtime buffer
    int buffer_size;
    dev_t devt;     // device number
    struct cdev cdev;
};

/* Simple file operations for character driver */
static int pseudo_open(struct inode *inode, struct file *file)
{
    struct pseudo_driver_data *drvdata;

    drvdata = container_of(inode->i_cdev, struct pseudo_driver_data, cdev);
    file->private_data = drvdata;

    pr_info("Pseudo driver: device opened\n");
    return 0;
}

static int pseudo_release(struct inode *inode, struct file *file)
{
    pr_info("Pseudo driver: device closed\n");
    return 0;
}

static ssize_t pseudo_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    struct pseudo_driver_data *drvdata = file->private_data;
    size_t to_copy;

    if (*ppos >= drvdata->buffer_size)
        return 0;

    to_copy = min(count, (size_t)(drvdata->buffer_size - *ppos));

    if (copy_to_user(buf, drvdata->buffer + *ppos, to_copy))
        return -EFAULT;

    *ppos += to_copy;
    return to_copy;
}

static ssize_t pseudo_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    struct pseudo_driver_data *drvdata = file->private_data;
    size_t to_copy;

    if (*ppos >= drvdata->buffer_size)
        return -ENOSPC;

    to_copy = min(count, (size_t)(drvdata->buffer_size - *ppos));

    if (copy_from_user(drvdata->buffer + *ppos, buf, to_copy))
        return -EFAULT;

    *ppos += to_copy;
    return to_copy;
}

static const struct file_operations pseudo_fops = {
    .owner   = THIS_MODULE,
    .open    = pseudo_open,
    .release = pseudo_release,
    .read    = pseudo_read,
    .write   = pseudo_write,
};

/* Probe function: called when device and driver match */
static int pseudo_probe(struct platform_device *pdev)
{
    struct pseudo_platform_data *pdata = pdev->dev.platform_data;
    struct pseudo_driver_data *drvdata;
    int ret;

    pr_info("Pseudo driver: probe called for %s\n", pdata->device_name);

    /* Allocate driver private data */
    drvdata = devm_kzalloc(&pdev->dev, sizeof(*drvdata), GFP_KERNEL);
    if (!drvdata)
        return -ENOMEM;

    drvdata->buffer_size = pdata->buffer_size;
    drvdata->buffer = devm_kzalloc(&pdev->dev, drvdata->buffer_size, GFP_KERNEL);
    if (!drvdata->buffer)
        return -ENOMEM;

    /* Allocate device number */
    ret = alloc_chrdev_region(&drvdata->devt, 0, 1, pdata->device_name);
    if (ret < 0) {
        pr_err("Pseudo driver: failed to allocate chrdev region\n");
        return ret;
    }

    /* Init cdev */
    cdev_init(&drvdata->cdev, &pseudo_fops);
    drvdata->cdev.owner = THIS_MODULE;

    ret = cdev_add(&drvdata->cdev, drvdata->devt, 1);
    if (ret) {
        pr_err("Pseudo driver: cdev_add failed\n");
        unregister_chrdev_region(drvdata->devt, 1);
        return ret;
    }

    /* Save driver data inside device */
    platform_set_drvdata(pdev, drvdata);

    pr_info("Pseudo driver: registered /dev with major=%d minor=%d\n",
            MAJOR(drvdata->devt), MINOR(drvdata->devt));

    return 0;
}

/* Remove function: cleanup when device is removed */
static int pseudo_remove(struct platform_device *pdev)
{
    struct pseudo_driver_data *drvdata = platform_get_drvdata(pdev);

    pr_info("Pseudo driver: remove called\n");

    cdev_del(&drvdata->cdev);
    unregister_chrdev_region(drvdata->devt, 1);

    return 0;
}

/* Platform driver structure */
static struct platform_driver pseudo_driver = {
    .probe  = pseudo_probe,
    .remove = pseudo_remove,
    .driver = {
        .name = "pseudo_char_driver",
        .owner = THIS_MODULE,
    },
};

module_platform_driver(pseudo_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ragab");
MODULE_DESCRIPTION("Pseudo Platform Character Driver Example");

// // pseudo_driver.c
// #include <linux/module.h>
// #include <linux/platform_device.h>
// #include <linux/fs.h>
// #include <linux/cdev.h>
// #include <linux/uaccess.h>

// #define PSEUDO_DEVNAME "pseudo_char"

// struct pseudo_platform_data {
//     int led_gpio;
//     const char *label;
// };

// /* Per-device driver data (kept in pdev->dev.driver_data) */
// struct pseudo_drvdata {
//     struct cdev cdev;
//     dev_t devt;
//     struct class *class;
//     char buffer[128];
// };

// /* File operations for the character device */
// static ssize_t pseudo_read(struct file *filp, char __user *buf,
//                            size_t count, loff_t *ppos)
// {
//     struct pseudo_drvdata *drvdata = filp->private_data;
//     size_t len = strlen(drvdata->buffer);

//     if (*ppos >= len)
//         return 0; // EOF

//     if (count > len - *ppos)
//         count = len - *ppos;

//     if (copy_to_user(buf, drvdata->buffer + *ppos, count))
//         return -EFAULT;

//     *ppos += count;
//     return count;
// }

// static ssize_t pseudo_write(struct file *filp, const char __user *buf,
//                             size_t count, loff_t *ppos)
// {
//     struct pseudo_drvdata *drvdata = filp->private_data;

//     if (count >= sizeof(drvdata->buffer))
//         count = sizeof(drvdata->buffer) - 1;

//     if (copy_from_user(drvdata->buffer, buf, count))
//         return -EFAULT;

//     drvdata->buffer[count] = '\0';
//     pr_info("pseudo_driver: written '%s'\n", drvdata->buffer);

//     return count;
// }

// static int pseudo_open(struct inode *inode, struct file *filp)
// {
//     struct pseudo_drvdata *drvdata =
//         container_of(inode->i_cdev, struct pseudo_drvdata, cdev);

//     filp->private_data = drvdata;
//     return 0;
// }

// static int pseudo_release(struct inode *inode, struct file *filp)
// {
//     return 0;
// }

// static const struct file_operations pseudo_fops = {
//     .owner   = THIS_MODULE,
//     .open    = pseudo_open,
//     .release = pseudo_release,
//     .read    = pseudo_read,
//     .write   = pseudo_write,
// };

// /* Platform driver probe */
// static int pseudo_probe(struct platform_device *pdev)
// {
//     struct pseudo_platform_data *pdata = dev_get_platdata(&pdev->dev);
//     struct pseudo_drvdata *drvdata;
//     int ret;

//     pr_info("pseudo_driver: probe for %s\n", dev_name(&pdev->dev));
//     if (pdata)
//         pr_info("pseudo_driver: got platform_data (gpio=%d, label=%s)\n",
//                 pdata->led_gpio, pdata->label);

//     /* Allocate driver private data */
//     drvdata = devm_kzalloc(&pdev->dev, sizeof(*drvdata), GFP_KERNEL);
//     if (!drvdata)
//         return -ENOMEM;

//     /* Allocate char device number */
//     ret = alloc_chrdev_region(&drvdata->devt, 0, 1, PSEUDO_DEVNAME);
//     if (ret)
//         return ret;

//     /* Init char device */
//     cdev_init(&drvdata->cdev, &pseudo_fops);
//     drvdata->cdev.owner = THIS_MODULE;
//     ret = cdev_add(&drvdata->cdev, drvdata->devt, 1);
//     if (ret)
//         goto unregister_chrdev;

//     /* Create class and device node (/dev/pseudo_char) */
//     drvdata->class = class_create(THIS_MODULE, PSEUDO_DEVNAME);
//     if (IS_ERR(drvdata->class)) {
//         ret = PTR_ERR(drvdata->class);
//         goto del_cdev;
//     }
//     device_create(drvdata->class, NULL, drvdata->devt, NULL, PSEUDO_DEVNAME);

//     /* Save drvdata in platform device (driver_data) */
//     platform_set_drvdata(pdev, drvdata);

//     strcpy(drvdata->buffer, "Hello from pseudo driver!\n");

//     return 0;

// del_cdev:
//     cdev_del(&drvdata->cdev);
// unregister_chrdev:
//     unregister_chrdev_region(drvdata->devt, 1);
//     return ret;
// }

// /* Platform driver remove */
// static int pseudo_remove(struct platform_device *pdev)
// {
//     struct pseudo_drvdata *drvdata = platform_get_drvdata(pdev);

//     device_destroy(drvdata->class, drvdata->devt);
//     class_destroy(drvdata->class);
//     cdev_del(&drvdata->cdev);
//     unregister_chrdev_region(drvdata->devt, 1);

//     pr_info("pseudo_driver: removed %s\n", dev_name(&pdev->dev));
//     return 0;
// }

// /* Platform driver struct */
// static struct platform_driver pseudo_driver = {
//     .probe  = pseudo_probe,
//     .remove = pseudo_remove,
//     .driver = {
//         .name  = "pseudo_device",  // must match device name
//         .owner = THIS_MODULE,
//     },
// };

// module_platform_driver(pseudo_driver);

// MODULE_LICENSE("GPL");
// MODULE_AUTHOR("Ragab");
// MODULE_DESCRIPTION("Pseudo Platform Driver (Character Device)");
