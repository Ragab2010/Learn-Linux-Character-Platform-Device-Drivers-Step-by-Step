// my_devices.c - using struct device + device_attribute
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/string.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Example Author");
MODULE_DESCRIPTION("Toy class + device + device_attribute example");
MODULE_VERSION("0.2");

struct my_dev {
    struct device *dev;
    int value;
    char status[16];
};

/* Global class */
static struct class *my_class;

/* Pointers to our devices */
static struct my_dev *dev1, *dev2;

/* --- sysfs attribute callbacks --- */
static ssize_t status_show(struct device *dev,
                           struct device_attribute *attr, char *buf)
{
    struct my_dev *mdev = dev_get_drvdata(dev);
    return scnprintf(buf, PAGE_SIZE, "%s\n", mdev->status);
}
static DEVICE_ATTR_RO(status);

static ssize_t value_show(struct device *dev,
                          struct device_attribute *attr, char *buf)
{
    struct my_dev *mdev = dev_get_drvdata(dev);
    return scnprintf(buf, PAGE_SIZE, "%d\n", mdev->value);
}

static ssize_t value_store(struct device *dev,
                           struct device_attribute *attr,
                           const char *buf, size_t count)
{
    struct my_dev *mdev = dev_get_drvdata(dev);
    long v;
    if (kstrtol(buf, 0, &v))
        return -EINVAL;
    mdev->value = (int)v;
    return count;
}
static DEVICE_ATTR_RW(value);

/* Group attributes together */
static struct attribute *my_attrs[] = {
    &dev_attr_status.attr,
    &dev_attr_value.attr,
    NULL,
};
ATTRIBUTE_GROUPS(my);

/* --- Module init/exit --- */
static int __init my_module_init(void)
{
    pr_info("my_devices: init\n");

    /* Create /sys/class/my_devices */
    my_class = class_create(THIS_MODULE, "my_devices");
    if (IS_ERR(my_class))
        return PTR_ERR(my_class);

    /* Attach default attribute groups for all devices in this class */
    my_class->dev_groups = my_groups;

    /* Create dev1 */
    dev1 = kzalloc(sizeof(*dev1), GFP_KERNEL);
    if (!dev1)
        return -ENOMEM;
    dev1->value = 1;
    strscpy(dev1->status, "OK", sizeof(dev1->status));

    dev1->dev = device_create(my_class, NULL, 0, dev1, "dev1");
    if (IS_ERR(dev1->dev)) {
        kfree(dev1);
        return PTR_ERR(dev1->dev);
    }
    dev_set_drvdata(dev1->dev, dev1);

    /* Create dev2 */
    dev2 = kzalloc(sizeof(*dev2), GFP_KERNEL);
    if (!dev2)
        return -ENOMEM;
    dev2->value = 42;
    strscpy(dev2->status, "OK", sizeof(dev2->status));

    dev2->dev = device_create(my_class, NULL, 0, dev2, "dev2");
    if (IS_ERR(dev2->dev)) {
        kfree(dev2);
        return PTR_ERR(dev2->dev);
    }
    dev_set_drvdata(dev2->dev, dev2);

    pr_info("my_devices: created /sys/class/my_devices/dev1 and dev2\n");
    return 0;
}

static void __exit my_module_exit(void)
{
    pr_info("my_devices: exit\n");

    if (dev2 && dev2->dev) {
        device_unregister(dev2->dev);
        kfree(dev2);
    }
    if (dev1 && dev1->dev) {
        device_unregister(dev1->dev);
        kfree(dev1);
    }

    if (my_class)
        class_destroy(my_class);

    pr_info("my_devices: cleaned up\n");
}

module_init(my_module_init);
module_exit(my_module_exit);
