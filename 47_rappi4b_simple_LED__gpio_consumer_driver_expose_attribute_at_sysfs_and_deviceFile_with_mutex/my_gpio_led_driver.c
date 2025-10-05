// my_gpio_led_driver.c
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/gpio/consumer.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>

#define DRIVER_NAME "myled"

struct myled_data {
    struct gpio_desc *gpiod;
    struct class *class;
    struct device *dev;
    struct cdev cdev;
    dev_t devt;
    struct mutex lock;
    int value;
};

static struct myled_data *g_data;

//
// ───────────────────────────── SYSFS ATTRIBUTES ─────────────────────────────
//

// Read LED value
static ssize_t value_show(struct device *dev,
                          struct device_attribute *attr, char *buf)
{
    struct myled_data *data = dev_get_drvdata(dev);
    int val;

    mutex_lock(&data->lock);
    val = gpiod_get_value(data->gpiod);
    data->value = val;
    mutex_unlock(&data->lock);

    return sprintf(buf, "%d\n", val);
}

// Write LED value
static ssize_t value_store(struct device *dev,
                           struct device_attribute *attr,
                           const char *buf, size_t count)
{
    struct myled_data *data = dev_get_drvdata(dev);
    int new_value;

    if (kstrtoint(buf, 10, &new_value))
        return -EINVAL;

    if (new_value != 0 && new_value != 1)
        return -EINVAL;

    mutex_lock(&data->lock);
    gpiod_set_value(data->gpiod, new_value);
    data->value = new_value;
    mutex_unlock(&data->lock);

    dev_info(dev, "LED set to %d (via sysfs)\n", new_value);

    return count;
}

// Direction attribute (for completeness)
static ssize_t direction_show(struct device *dev,
                              struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "out\n"); // LED is output only
}

static DEVICE_ATTR_RW(value);
static DEVICE_ATTR_RO(direction);

static struct attribute *myled_attrs[] = {
    &dev_attr_value.attr,
    &dev_attr_direction.attr,
    NULL,
};

static const struct attribute_group myled_attr_group = {
    .attrs = myled_attrs,
};

//
// ───────────────────────────── CHARACTER DEVICE ─────────────────────────────
//

static int myled_open(struct inode *inode, struct file *file)
{
    struct myled_data *data = container_of(inode->i_cdev, struct myled_data, cdev);
    file->private_data = data;
    return 0;
}

static ssize_t myled_read(struct file *file, char __user *buf,
                          size_t len, loff_t *off)
{
    struct myled_data *data = file->private_data;
    char tmp[4];
    int val;
    int n;

    mutex_lock(&data->lock);
    val = gpiod_get_value(data->gpiod);
    data->value = val;
    mutex_unlock(&data->lock);

    n = snprintf(tmp, sizeof(tmp), "%d\n", val);
    if (*off >= n)
        return 0;
    if (len > n - *off)
        len = n - *off;
    if (copy_to_user(buf, tmp + *off, len))
        return -EFAULT;

    *off += len;
    return len;
}

static ssize_t myled_write(struct file *file, const char __user *buf,
                           size_t len, loff_t *off)
{
    struct myled_data *data = file->private_data;
    char tmp[8];
    int val;

    if (len >= sizeof(tmp))
        return -EINVAL;

    if (copy_from_user(tmp, buf, len))
        return -EFAULT;
    tmp[len] = '\0';

    if (kstrtoint(tmp, 10, &val))
        return -EINVAL;
    if (val != 0 && val != 1)
        return -EINVAL;

    mutex_lock(&data->lock);
    gpiod_set_value(data->gpiod, val);
    data->value = val;
    mutex_unlock(&data->lock);

    dev_info(data->dev, "LED set to %d (via /dev)\n", val);

    return len;
}

static const struct file_operations myled_fops = {
    .owner = THIS_MODULE,
    .open = myled_open,
    .read = myled_read,
    .write = myled_write,
};

//
// ───────────────────────────── PLATFORM DRIVER ─────────────────────────────
//

static int myled_probe(struct platform_device *pdev)
{
    int ret;
    struct myled_data *data;

    dev_info(&pdev->dev, "Probing myled platform driver...\n");

    data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    g_data = data;
    mutex_init(&data->lock);
    data->value = 0;

    // 1. Get GPIO from device tree
    data->gpiod = devm_gpiod_get(&pdev->dev, NULL, GPIOD_OUT_LOW);
    if (IS_ERR(data->gpiod)) {
        dev_err(&pdev->dev, "Failed to get GPIO from DT\n");
        return PTR_ERR(data->gpiod);
    }

    // 2. Create class
    data->class = class_create(THIS_MODULE, DRIVER_NAME);
    if (IS_ERR(data->class))
        return PTR_ERR(data->class);

    // 3. Allocate char device region
    ret = alloc_chrdev_region(&data->devt, 0, 1, DRIVER_NAME);
    if (ret)
        goto err_class;

    // 4. Init and add cdev
    cdev_init(&data->cdev, &myled_fops);
    data->cdev.owner = THIS_MODULE;
    ret = cdev_add(&data->cdev, data->devt, 1);
    if (ret)
        goto err_chrdev;

    // 5. Create device node in /dev
    data->dev = device_create(data->class, NULL, data->devt,
                              NULL, DRIVER_NAME);
    if (IS_ERR(data->dev)) {
        ret = PTR_ERR(data->dev);
        goto err_cdev;
    }

    dev_set_drvdata(data->dev, data);

    // 6. Create sysfs attributes
    ret = sysfs_create_group(&data->dev->kobj, &myled_attr_group);
    if (ret)
        goto err_device;

    dev_info(&pdev->dev, "myled: driver ready\n");
    return 0;

err_device:
    device_destroy(data->class, data->devt);
err_cdev:
    cdev_del(&data->cdev);
err_chrdev:
    unregister_chrdev_region(data->devt, 1);
err_class:
    class_destroy(data->class);
    return ret;
}

static int myled_remove(struct platform_device *pdev)
{
    struct myled_data *data = g_data;

    sysfs_remove_group(&data->dev->kobj, &myled_attr_group);
    device_destroy(data->class, data->devt);
    cdev_del(&data->cdev);
    unregister_chrdev_region(data->devt, 1);
    class_destroy(data->class);

    dev_info(&pdev->dev, "myled removed\n");
    return 0;
}

static const struct of_device_id myled_of_match[] = {
    { .compatible = "ragab,myled", },
    { },
};
MODULE_DEVICE_TABLE(of, myled_of_match);

static struct platform_driver myled_driver = {
    .probe = myled_probe,
    .remove = myled_remove,
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = myled_of_match,
    },
};

module_platform_driver(myled_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ragab Elsayed");
MODULE_DESCRIPTION("Platform LED driver with sysfs, /dev, and GPIO mutex protection");
