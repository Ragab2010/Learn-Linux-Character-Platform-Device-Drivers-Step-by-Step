#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/of.h>           // For device tree matching
#include <linux/gpio/consumer.h> // For gpiod interface
#include <linux/fs.h>
#include <linux/device.h>

// ---------------- GLOBALS ----------------

// Pointer to gpio descriptor (abstract representation of pin)
static struct gpio_desc *led_gpiod;

// The class for sysfs: /sys/class/myled/
static struct class *led_class;

// The device inside class: /sys/class/myled/led0
static struct device *led_device;


// ---------------- SYSFS ATTRIBUTE ----------------

// Sysfs attribute to control LED brightness
// echo 1 > /sys/class/myled/led0/brightness  --> LED ON
// echo 0 > /sys/class/myled/led0/brightness  --> LED OFF
// cat /sys/class/myled/led0/brightness       --> show current value

static ssize_t brightness_show(struct device *dev,
                               struct device_attribute *attr, char *buf)
{
    int value = gpiod_get_value(led_gpiod);   // read current pin state
    return sprintf(buf, "%d\n", value);       // print 0 or 1
}

static ssize_t brightness_store(struct device *dev,
                                struct device_attribute *attr,
                                const char *buf, size_t count)
{
    int val;
    if (kstrtoint(buf, 10, &val) < 0)  // convert user input string to int
        return -EINVAL;

    // Control the GPIO pin (0 = off, 1 = on)
    gpiod_set_value(led_gpiod, val ? 1 : 0);
    return count;
}

// Create a device_attribute struct (ties show/store to sysfs file)
static DEVICE_ATTR_RW(brightness);
// Equivalent to:
// struct device_attribute dev_attr_brightness = __ATTR_RW(brightness);



// ---------------- PROBE & REMOVE ----------------

static int myled_probe(struct platform_device *pdev)
{
    int ret;

    pr_info("myled: Probing LED driver\n");

    // 1. Request the GPIO from device tree
    led_gpiod = devm_gpiod_get(&pdev->dev, NULL, GPIOD_OUT_LOW);
    if (IS_ERR(led_gpiod)) {
        pr_err("myled: Failed to get GPIO from device tree\n");
        return PTR_ERR(led_gpiod);
    }

    // 2. Create class in /sys/class/
    led_class = class_create(THIS_MODULE, "myled");
    if (IS_ERR(led_class)) {
        pr_err("myled: Failed to create class\n");
        return PTR_ERR(led_class);
    }

    // 3. Create device inside class
    led_device = device_create(led_class, NULL, 0, NULL, "led0");
    if (IS_ERR(led_device)) {
        pr_err("myled: Failed to create device\n");
        class_destroy(led_class);
        return PTR_ERR(led_device);
    }

    // 4. Add sysfs file: /sys/class/myled/led0/brightness
    ret = device_create_file(led_device, &dev_attr_brightness);
    if (ret) {
        pr_err("myled: Failed to create sysfs attribute\n");
        device_destroy(led_class, 0);
        class_destroy(led_class);
        return ret;
    }

    pr_info("myled: Probe successful, LED sysfs ready\n");
    return 0;
}

static int myled_remove(struct platform_device *pdev)
{
    // Remove sysfs file
    device_remove_file(led_device, &dev_attr_brightness);

    // Remove device + class
    device_destroy(led_class, 0);
    class_destroy(led_class);

    pr_info("myled: Removed LED driver\n");
    return 0;
}



// ---------------- OF MATCH TABLE ----------------

static const struct of_device_id myled_of_match[] = {
    { .compatible = "ragab,myled" },
    { }
};
MODULE_DEVICE_TABLE(of, myled_of_match);



// ---------------- PLATFORM DRIVER ----------------

static struct platform_driver myled_driver = {
    .probe  = myled_probe,
    .remove = myled_remove,
    .driver = {
        .name           = "myled",
        .of_match_table = myled_of_match,
    },
};

module_platform_driver(myled_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ragab Elsayed");
MODULE_DESCRIPTION("GPIO LED consumer driver with sysfs control");
