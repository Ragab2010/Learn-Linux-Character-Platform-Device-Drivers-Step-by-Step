#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/gpio/consumer.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>

#define DRIVER_NAME "myled"

static dev_t devno;
static struct cdev myled_cdev;
static struct class *myled_class;
static struct device *myled_dev;
static struct gpio_desc *led_gpio;

/* -------------------------
 * Sysfs "direction" attribute
 * -------------------------
 */
static ssize_t direction_show(struct device *dev,
                              struct device_attribute *attr, char *buf)
{
    int dir = gpiod_get_direction(led_gpio);  // 0=out, 1=in, <0=error
    if (dir < 0)
        return dir;

    return sprintf(buf, "%s\n", dir == 0 ? "out" : "in");
}

static ssize_t direction_store(struct device *dev,
                               struct device_attribute *attr,
                               const char *buf, size_t count)
{
    if (sysfs_streq(buf, "out"))
        gpiod_direction_output(led_gpio, 0);
    else if (sysfs_streq(buf, "in"))
        gpiod_direction_input(led_gpio);
    else
        return -EINVAL;

    return count;
}
static DEVICE_ATTR_RW(direction);

/* -------------------------
 * Sysfs "value" attribute
 * -------------------------
 */
static ssize_t value_show(struct device *dev,
                          struct device_attribute *attr, char *buf)
{
    int val = gpiod_get_value(led_gpio);
    return sprintf(buf, "%d\n", val);
}

static ssize_t value_store(struct device *dev,
                           struct device_attribute *attr,
                           const char *buf, size_t count)
{
    unsigned long val;
    int ret = kstrtoul(buf, 0, &val);
    if (ret)
        return ret;

    if (val == 0 || val == 1)
        gpiod_set_value(led_gpio, val);
    else
        return -EINVAL;

    return count;
}
static DEVICE_ATTR_RW(value);

/* -------------------------
 * Character device write()
 * -------------------------
 */
static ssize_t myled_write(struct file *file, const char __user *buf,
                           size_t len, loff_t *ppos)
{
    char kbuf[2];
    if (len > 1) len = 1;
    if (copy_from_user(kbuf, buf, len))
        return -EFAULT;

    if (kbuf[0] == '1')
        gpiod_set_value(led_gpio, 1);
    else if (kbuf[0] == '0')
        gpiod_set_value(led_gpio, 0);

    return len;
}

static struct file_operations myled_fops = {
    .owner = THIS_MODULE,
    .write = myled_write,
};

/* -------------------------
 * Probe / Remove
 * -------------------------
 */
static int myled_probe(struct platform_device *pdev)
{
    int ret;

    // Get LED GPIO from DT
    led_gpio = devm_gpiod_get(&pdev->dev, "led", GPIOD_OUT_LOW);
    if (IS_ERR(led_gpio))
        return PTR_ERR(led_gpio);

    // Allocate char dev
    ret = alloc_chrdev_region(&devno, 0, 1, DRIVER_NAME);
    if (ret < 0) return ret;

    cdev_init(&myled_cdev, &myled_fops);
    ret = cdev_add(&myled_cdev, devno, 1);
    if (ret) goto unregister_region;

    myled_class = class_create(THIS_MODULE, "myled");
    if (IS_ERR(myled_class)) {
        ret = PTR_ERR(myled_class);
        goto del_cdev;
    }

    myled_dev = device_create(myled_class, NULL, devno, NULL, "led0");
    if (IS_ERR(myled_dev)) {
        ret = PTR_ERR(myled_dev);
        goto destroy_class;
    }

    // Create sysfs attributes
    ret = device_create_file(myled_dev, &dev_attr_direction);
    if (ret) goto destroy_device;
    ret = device_create_file(myled_dev, &dev_attr_value);
    if (ret) goto remove_direction;

    dev_info(&pdev->dev, "myled driver probed\n");
    return 0;

remove_direction:
    device_remove_file(myled_dev, &dev_attr_direction);
destroy_device:
    device_destroy(myled_class, devno);
destroy_class:
    class_destroy(myled_class);
del_cdev:
    cdev_del(&myled_cdev);
unregister_region:
    unregister_chrdev_region(devno, 1);
    return ret;
}

static int myled_remove(struct platform_device *pdev)
{
    device_remove_file(myled_dev, &dev_attr_value);
    device_remove_file(myled_dev, &dev_attr_direction);
    device_destroy(myled_class, devno);
    class_destroy(myled_class);
    cdev_del(&myled_cdev);
    unregister_chrdev_region(devno, 1);
    return 0;
}

/* -------------------------
 * Device tree binding
 * -------------------------
 */
static const struct of_device_id myled_dt_ids[] = {
    { .compatible = "ragab,myled" },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, myled_dt_ids);

static struct platform_driver myled_driver = {
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = myled_dt_ids,
    },
    .probe = myled_probe,
    .remove = myled_remove,
};
module_platform_driver(myled_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ragab");
MODULE_DESCRIPTION("Platform LED driver with direction and value sysfs attributes");
