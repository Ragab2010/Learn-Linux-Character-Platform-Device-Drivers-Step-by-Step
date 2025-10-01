#include <linux/module.h>
#include <linux/platform_device.h>

/* Platform data structure */
struct pseudo_platform_data {
    int buffer_size;
    const char *device_name;
};

/* Three instances of platform data */
static struct pseudo_platform_data pdata0 = {
    .buffer_size = 64,
    .device_name = "pseudo_char_dev0",
};

static struct pseudo_platform_data pdata1 = {
    .buffer_size = 128,
    .device_name = "pseudo_char_dev1",
};

static struct pseudo_platform_data pdata2 = {
    .buffer_size = 256,
    .device_name = "pseudo_char_dev2",
};

/* Array of devices */
static struct platform_device *pdevs[3];

static int __init pseudo_device_init(void)
{
    pr_info("Pseudo device: init (creating 3 devices)\n");

    /* Device 0 */
    pdevs[0] = platform_device_alloc("pseudo_char_driver", 0);
    if (!pdevs[0])
        return -ENOMEM;
    pdevs[0]->dev.platform_data = &pdata0;
    platform_device_add(pdevs[0]);

    /* Device 1 */
    pdevs[1] = platform_device_alloc("pseudo_char_driver", 1);
    if (!pdevs[1])
        return -ENOMEM;
    pdevs[1]->dev.platform_data = &pdata1;
    platform_device_add(pdevs[1]);

    /* Device 2 */
    pdevs[2] = platform_device_alloc("pseudo_char_driver", 2);
    if (!pdevs[2])
        return -ENOMEM;
    pdevs[2]->dev.platform_data = &pdata2;
    platform_device_add(pdevs[2]);

    return 0;
}

static void __exit pseudo_device_exit(void)
{
    pr_info("Pseudo device: exit (removing 3 devices)\n");

    platform_device_unregister(pdevs[0]);
    platform_device_unregister(pdevs[1]);
    platform_device_unregister(pdevs[2]);
}

module_init(pseudo_device_init);
module_exit(pseudo_device_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ragab");
MODULE_DESCRIPTION("Pseudo Platform Devices (3 instances)");
