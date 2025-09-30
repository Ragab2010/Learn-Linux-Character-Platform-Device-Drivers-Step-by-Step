#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

struct pseudo_platform_data {
    int some_value;
    char label[20];
};

/* Private data per device */
struct pseudo_dev {
    struct pseudo_platform_data pdata;
    struct miscdevice miscdev;   // Each node → one /dev/pseudoX
};

/* File operations */
static int pseudo_open(struct inode *inode, struct file *file)
{
    struct pseudo_dev *priv =
        container_of(file->private_data, struct pseudo_dev, miscdev);
    pr_info("pseudo: open device %s (val=%d)\n",
            priv->pdata.label, priv->pdata.some_value);
    return 0;
}

static ssize_t pseudo_read(struct file *file, char __user *buf,
                           size_t count, loff_t *ppos)
{
    struct pseudo_dev *priv =
        container_of(file->private_data, struct pseudo_dev, miscdev);
    char msg[64];
    int len;

    len = snprintf(msg, sizeof(msg),
                   "Label=%s, Value=%d\n",
                   priv->pdata.label,
                   priv->pdata.some_value);

    if (*ppos >= len)
        return 0;

    if (copy_to_user(buf, msg, len))
        return -EFAULT;

    *ppos += len;
    return len;
}

static ssize_t pseudo_write(struct file *file, const char __user *buf,
                            size_t count, loff_t *ppos)
{
    char kbuf[64];
    if (count >= sizeof(kbuf))
        return -EINVAL;

    if (copy_from_user(kbuf, buf, count))
        return -EFAULT;

    kbuf[count] = '\0';
    pr_info("pseudo: write got \"%s\"\n", kbuf);
    return count;
}

static const struct file_operations pseudo_fops = {
    .owner = THIS_MODULE,
    .open  = pseudo_open,
    .read  = pseudo_read,
    .write = pseudo_write,
};

/* Match table for DT */
static const struct of_device_id pseudo_of_match[] = {
    { .compatible = "myvendor,pseudo" },
    {},
};
MODULE_DEVICE_TABLE(of, pseudo_of_match);

/* Probe called once per DT node */
static int pseudo_probe(struct platform_device *pdev)
{
    struct pseudo_dev *priv;
    const char *label;
    u32 value;
    static int dev_idx;  // create unique /dev names
    int ret;

    pr_info("pseudo: probe for node %pOF\n", pdev->dev.of_node);

    /* Allocate memory for device */
    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    /* Read platform data from DT */
    if (of_property_read_string(pdev->dev.of_node, "label", &label))
        label = "unknown";

    of_property_read_u32(pdev->dev.of_node, "some-value", &value);

    strscpy(priv->pdata.label, label, sizeof(priv->pdata.label));
    priv->pdata.some_value = value;

    /* Setup miscdevice (/dev/pseudoX) */
    priv->miscdev.minor = MISC_DYNAMIC_MINOR;
    priv->miscdev.name = kasprintf(GFP_KERNEL, "pseudo%d", dev_idx++);
    priv->miscdev.fops = &pseudo_fops;

    ret = misc_register(&priv->miscdev);
    if (ret) {
        pr_err("pseudo: failed to register misc device\n");
        return ret;
    }

    platform_set_drvdata(pdev, priv);
    pr_info("pseudo: registered %s as /dev/%s\n",
            priv->pdata.label, priv->miscdev.name);
    return 0;
}

static int pseudo_remove(struct platform_device *pdev)
{
    struct pseudo_dev *priv = platform_get_drvdata(pdev);
    misc_deregister(&priv->miscdev);
    pr_info("pseudo: removed /dev/%s\n", priv->miscdev.name);
    return 0;
}

static struct platform_driver pseudo_driver = {
    .driver = {
        .name = "pseudo_driver",
        .of_match_table = pseudo_of_match,
    },
    .probe = pseudo_probe,
    .remove = pseudo_remove,
};

module_platform_driver(pseudo_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("Pseudo platform driver with DT overlay and misc devices");
