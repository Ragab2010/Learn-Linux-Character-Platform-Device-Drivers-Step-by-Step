#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/gpio/consumer.h>

#define DRIVER_NAME "myled"

static dev_t devno;
static struct cdev myled_cdev;
static struct class *myled_class;
static struct device *myled_dev;
static struct gpio_desc *led_gpio;

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

static int myled_probe(struct platform_device *pdev)
{
    int ret;

    // Get LED GPIO from DT
    led_gpio = devm_gpiod_get(&pdev->dev, "led", GPIOD_OUT_LOW);
    if (IS_ERR(led_gpio))
        return PTR_ERR(led_gpio);

    // Allocate char device region
    ret = alloc_chrdev_region(&devno, 0, 1, DRIVER_NAME);
    if (ret < 0) return ret;

    // Init and add cdev
    cdev_init(&myled_cdev, &myled_fops);
    ret = cdev_add(&myled_cdev, devno, 1);
    if (ret) goto unregister_region;

    // Create class (for /sys/class/myled)
    myled_class = class_create(THIS_MODULE, "myled");
    if (IS_ERR(myled_class)) {
        ret = PTR_ERR(myled_class);
        goto del_cdev;
    }

    // Create device (appears in /dev/myled0 and /sys/class/myled/myled0)
    myled_dev = device_create(myled_class, NULL, devno, NULL, "myled0");
    if (IS_ERR(myled_dev)) {
        ret = PTR_ERR(myled_dev);
        goto destroy_class;
    }

    dev_info(&pdev->dev, "myled probed successfully\n");
    return 0;

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
    device_destroy(myled_class, devno);
    class_destroy(myled_class);
    cdev_del(&myled_cdev);
    unregister_chrdev_region(devno, 1);
    return 0;
}

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
