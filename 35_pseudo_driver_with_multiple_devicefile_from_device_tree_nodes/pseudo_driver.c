/*
 * pseudo_driver_dt_char.c
 *
 * Device Tree based platform driver that:
 *  - Matches "mycompany,pseudo-char" nodes from DT
 *  - Creates one char device per DT node under /dev/pseudoX
 *  - Each device file has its own driver_data with DT properties
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define DRIVER_NAME "pseudo-char-dt"
#define DEVICE_NAME "pseudo"

/* Per-device DT data */
struct pseudo_platform_data {
    int value;
    const char *label;
};

/* Driver runtime data per device */
struct pseudo_driver_data {
    int device_index;
    struct pseudo_platform_data pdata;

    struct cdev cdev;             /* character device object */
    dev_t devt;                   /* device number (major+minor) */
    struct class *class;          /* class for udev auto-create */
    struct device *device;        /* device entry in /dev/ */
};

/* Global major number (all minors allocated dynamically) */
static dev_t pseudo_base_dev;
static struct class *pseudo_class;

/* -------------------- File operations -------------------- */
static int pseudo_open(struct inode *inode, struct file *file)
{
    struct pseudo_driver_data *drvdata;

    drvdata = container_of(inode->i_cdev, struct pseudo_driver_data, cdev);
    file->private_data = drvdata;

    pr_info("pseudo: open device index=%d label=%s value=%d\n",
            drvdata->device_index, drvdata->pdata.label, drvdata->pdata.value);

    return 0;
}

static int pseudo_release(struct inode *inode, struct file *file)
{
    struct pseudo_driver_data *drvdata = file->private_data;
    pr_info("pseudo: release device index=%d\n", drvdata->device_index);
    return 0;
}

static ssize_t pseudo_read(struct file *file, char __user *buf,
                           size_t count, loff_t *ppos)
{
    struct pseudo_driver_data *drvdata = file->private_data;
    char buffer[128];
    int len;

    len = snprintf(buffer, sizeof(buffer),
                   "index=%d label=%s value=%d\n",
                   drvdata->device_index, drvdata->pdata.label,
                   drvdata->pdata.value);

    return simple_read_from_buffer(buf, count, ppos, buffer, len);
}

static ssize_t pseudo_write(struct file *file, const char __user *buf,
                            size_t count, loff_t *ppos)
{
    struct pseudo_driver_data *drvdata = file->private_data;
    char kbuf[32];

    if (count >= sizeof(kbuf))
        return -EINVAL;

    if (copy_from_user(kbuf, buf, count))
        return -EFAULT;
    kbuf[count] = '\0';

    /* Overwrite value via echo to device */
    if (kstrtoint(kbuf, 10, &drvdata->pdata.value) == 0)
        pr_info("pseudo: device index=%d new value=%d\n",
                drvdata->device_index, drvdata->pdata.value);
    else
        pr_warn("pseudo: invalid write string\n");

    return count;
}

static const struct file_operations pseudo_fops = {
    .owner = THIS_MODULE,
    .open = pseudo_open,
    .release = pseudo_release,
    .read = pseudo_read,
    .write = pseudo_write,
};

/* -------------------- DT match table -------------------- */
static const struct of_device_id pseudo_of_match[] = {
    { .compatible = "mycompany,pseudo-char", .data = (void *)1 },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, pseudo_of_match);

/* -------------------- Probe -------------------- */
static int pseudo_probe(struct platform_device *pdev)
{
    struct pseudo_driver_data *drvdata;
    struct device_node *np = pdev->dev.of_node;
    int ret;
    static int minor = 0; /* allocate increasing minors */

    if (!np)
        return -ENODEV;

    drvdata = devm_kzalloc(&pdev->dev, sizeof(*drvdata), GFP_KERNEL);
    if (!drvdata)
        return -ENOMEM;

    /* Parse DT properties */
    ret = of_property_read_u32(np, "value", &drvdata->pdata.value);
    if (ret)
        drvdata->pdata.value = 0;

    ret = of_property_read_string(np, "label", &drvdata->pdata.label);
    if (ret)
        drvdata->pdata.label = "unknown";

    drvdata->device_index = minor; /* assign sequential index */
    drvdata->class = pseudo_class;
    drvdata->devt = MKDEV(MAJOR(pseudo_base_dev), minor);

    /* Register char device */
    cdev_init(&drvdata->cdev, &pseudo_fops);
    drvdata->cdev.owner = THIS_MODULE;

    ret = cdev_add(&drvdata->cdev, drvdata->devt, 1);
    if (ret) {
        dev_err(&pdev->dev, "Failed to add cdev\n");
        return ret;
    }

    drvdata->device = device_create(pseudo_class, &pdev->dev,
                                    drvdata->devt, NULL,
                                    DEVICE_NAME "%d", minor);
    if (IS_ERR(drvdata->device)) {
        cdev_del(&drvdata->cdev);
        return PTR_ERR(drvdata->device);
    }

    dev_set_drvdata(&pdev->dev, drvdata);

    dev_info(&pdev->dev, "Created /dev/%s%d label=%s value=%d\n",
             DEVICE_NAME, minor, drvdata->pdata.label, drvdata->pdata.value);

    minor++;
    return 0;
}

/* -------------------- Remove -------------------- */
static int pseudo_remove(struct platform_device *pdev)
{
    struct pseudo_driver_data *drvdata = dev_get_drvdata(&pdev->dev);

    device_destroy(drvdata->class, drvdata->devt);
    cdev_del(&drvdata->cdev);

    dev_info(&pdev->dev, "Removed /dev/%s%d\n",
             DEVICE_NAME, drvdata->device_index);

    return 0;
}

/* -------------------- Platform driver -------------------- */
static struct platform_driver pseudo_driver = {
    .probe = pseudo_probe,
    .remove = pseudo_remove,
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = pseudo_of_match,
    },
};

/* -------------------- Init/Exit -------------------- */
static int __init pseudo_init(void)
{
    int ret;

    /* Allocate a range of device numbers (major) */
    ret = alloc_chrdev_region(&pseudo_base_dev, 0, 10, DEVICE_NAME);
    if (ret)
        return ret;

    /* Create sysfs class for /dev auto-create */
    pseudo_class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(pseudo_class)) {
        unregister_chrdev_region(pseudo_base_dev, 10);
        return PTR_ERR(pseudo_class);
    }

    return platform_driver_register(&pseudo_driver);
}

static void __exit pseudo_exit(void)
{
    platform_driver_unregister(&pseudo_driver);
    class_destroy(pseudo_class);
    unregister_chrdev_region(pseudo_base_dev, 10);
}

module_init(pseudo_init);
module_exit(pseudo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ragab");
MODULE_DESCRIPTION("Pseudo char platform driver with Device Tree nodes");
