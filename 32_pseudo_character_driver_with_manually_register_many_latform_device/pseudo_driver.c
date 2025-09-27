#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

struct pseudo_platform_data {
    int buffer_size;
    const char *device_name;
};

/* Driver private data per device */
struct pseudo_driver_data {
    char *buffer;            // runtime buffer
    int buffer_size;
    dev_t devt;              // device number
    struct cdev cdev;        // char device
    struct class *class;     // device class (shared)
    struct device *device;   // device node (/dev/..)
};

/* File ops */
static int pseudo_open(struct inode *inode, struct file *file)
{
    struct pseudo_driver_data *drvdata;

    drvdata = container_of(inode->i_cdev, struct pseudo_driver_data, cdev);
    file->private_data = drvdata;

    pr_info("Pseudo driver: opened device %s\n", drvdata->device->kobj.name);
    return 0;
}

static int pseudo_release(struct inode *inode, struct file *file)
{
    pr_info("Pseudo driver: closed device\n");
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

/* Probe */
static int pseudo_probe(struct platform_device *pdev)
{
    struct pseudo_platform_data *pdata = pdev->dev.platform_data;
    struct pseudo_driver_data *drvdata;
    int ret;

    pr_info("Pseudo driver: probe called for %s (id=%d)\n",
            pdata->device_name, pdev->id);

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
        pr_err("Pseudo driver: failed to alloc chrdev region\n");
        return ret;
    }

    /* Init and add cdev */
    cdev_init(&drvdata->cdev, &pseudo_fops);
    drvdata->cdev.owner = THIS_MODULE;
    ret = cdev_add(&drvdata->cdev, drvdata->devt, 1);
    if (ret) {
        pr_err("Pseudo driver: cdev_add failed\n");
        unregister_chrdev_region(drvdata->devt, 1);
        return ret;
    }

    /* Create device class only once (static shared class) */
    static struct class *pseudo_class;
    if (!pseudo_class) {
        pseudo_class = class_create(THIS_MODULE, "pseudo_class");
        if (IS_ERR(pseudo_class)) {
            pr_err("Pseudo driver: class_create failed\n");
            cdev_del(&drvdata->cdev);
            unregister_chrdev_region(drvdata->devt, 1);
            return PTR_ERR(pseudo_class);
        }
    }
    drvdata->class = pseudo_class;

    /* Create /dev entry */
    drvdata->device = device_create(drvdata->class, NULL,
                                    drvdata->devt, NULL,
                                    pdata->device_name);
    if (IS_ERR(drvdata->device)) {
        pr_err("Pseudo driver: device_create failed\n");
        cdev_del(&drvdata->cdev);
        unregister_chrdev_region(drvdata->devt, 1);
        return PTR_ERR(drvdata->device);
    }

    /* Save driver data */
    platform_set_drvdata(pdev, drvdata);

    pr_info("Pseudo driver: /dev/%s created (major=%d minor=%d, buffer=%d)\n",
            pdata->device_name, MAJOR(drvdata->devt),
            MINOR(drvdata->devt), drvdata->buffer_size);

    return 0;
}

/* Remove */
static int pseudo_remove(struct platform_device *pdev)
{
    struct pseudo_driver_data *drvdata = platform_get_drvdata(pdev);

    pr_info("Pseudo driver: remove called for device\n");

    device_destroy(drvdata->class, drvdata->devt);
    cdev_del(&drvdata->cdev);
    unregister_chrdev_region(drvdata->devt, 1);

    return 0;
}

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
MODULE_DESCRIPTION("Pseudo Platform Driver (3 devices)");
