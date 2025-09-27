#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

/* 
 * Platform data → passed from the board/device to the driver.
 * This simulates configuration parameters (like GPIO numbers, buffer size, etc.).
 */
struct pseudo_platform_data {
    int buffer_size;
    const char *device_name;
};

/* Define the platform data for this pseudo device */
static struct pseudo_platform_data pseudo_pdata = {
    .buffer_size = 128,
    .device_name = "pseudo_char_dev",
};

/* Create the platform device */
static struct platform_device *pseudo_pdev;

static int __init pseudo_device_init(void)
{
    pr_info("Pseudo device: init\n");

    /* Allocate and register a platform device */
    pseudo_pdev = platform_device_alloc("pseudo_char_driver", -1);
    if (!pseudo_pdev)
        return -ENOMEM;

    /* Attach platform data to the device */
    pseudo_pdev->dev.platform_data = &pseudo_pdata;

    /* Register the device with the kernel */
    return platform_device_add(pseudo_pdev);
}

static void __exit pseudo_device_exit(void)
{
    pr_info("Pseudo device: exit\n");

    /* Unregister and free the device */
    platform_device_unregister(pseudo_pdev);
}

module_init(pseudo_device_init);
module_exit(pseudo_device_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ragab");
MODULE_DESCRIPTION("Pseudo Platform Device Example");


// // pseudo_device.c
// #include <linux/module.h>
// #include <linux/platform_device.h>

// /* Platform-specific data (passed to driver via pdev->dev.platform_data) */
// struct pseudo_platform_data {
//     int led_gpio;
//     const char *label;
// };

// /* Example platform data */
// static struct pseudo_platform_data pseudo_pdata = {
//     .led_gpio = 17,
//     .label    = "pseudo_led",
// };

// /* Fake resources */
// static struct resource pseudo_resources[] = {
//     {
//         .start = 0x10000000,
//         .end   = 0x10000fff,
//         .flags = IORESOURCE_MEM,
//     },
//     {
//         .start = 42,
//         .end   = 42,
//         .flags = IORESOURCE_IRQ,
//     },
// };

// static struct platform_device *pseudo_pdev;

// static int __init pseudo_device_init(void)
// {
//     pr_info("pseudo_device: init\n");

//     pseudo_pdev = platform_device_alloc("pseudo_device", -1);
//     if (!pseudo_pdev)
//         return -ENOMEM;

//     /* Add resources */
//     platform_device_add_resources(pseudo_pdev,
//                                   pseudo_resources,
//                                   ARRAY_SIZE(pseudo_resources));

//     /* Attach platform data */
//     pseudo_pdev->dev.platform_data = &pseudo_pdata;

//     /* Register device */
//     return platform_device_add(pseudo_pdev);
// }

// static void __exit pseudo_device_exit(void)
// {
//     pr_info("pseudo_device: exit\n");
//     platform_device_unregister(pseudo_pdev);
// }

// module_init(pseudo_device_init);
// module_exit(pseudo_device_exit);

// MODULE_LICENSE("GPL");
// MODULE_AUTHOR("Ragab");
// MODULE_DESCRIPTION("Pseudo Platform Device with platform_data");
