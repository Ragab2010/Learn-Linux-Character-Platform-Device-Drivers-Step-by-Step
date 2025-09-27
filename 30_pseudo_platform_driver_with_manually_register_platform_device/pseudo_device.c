// pseudo_device.c
#include <linux/module.h>
#include <linux/platform_device.h>

/*
 * Resources describe hardware aspects of the device:
 *  - IORESOURCE_MEM: MMIO base address (like registers region).
 *  - IORESOURCE_IRQ: Interrupt number assigned to the device.
 *
 * These are fake values since this is just a pseudo device.
 */
static struct resource pseudo_resources[] = {
    {
        .start = 0x10000000,      // pretend MMIO base address
        .end   = 0x10000fff,      // 4KB region (0x1000 bytes)
        .flags = IORESOURCE_MEM,  // memory-mapped I/O
    },
    {
        .start = 42,              // fake IRQ line
        .end   = 42,
        .flags = IORESOURCE_IRQ,  // interrupt resource
    },
};

/*
 * Pointer to the registered platform device.
 * We keep it global so we can unregister it in exit().
 */
static struct platform_device *pseudo_pdev;

/*
 * Module init: called when we insmod the module.
 *
 * We use platform_device_register_simple() because it's a shortcut
 * for allocating, initializing, and registering a platform_device.
 *   - First argument: device name (must match driver name later).
 *   - Second argument: ID (-1 = single instance).
 *   - Third/Fourth: resources array and its size.
 */
static int __init pseudo_device_init(void)
{
    pr_info("pseudo_device: init\n");

    pseudo_pdev = platform_device_register_simple(
        "pseudo_device",               // device name
        -1,                            // only one instance
        pseudo_resources,              // resource array
        ARRAY_SIZE(pseudo_resources)); // number of resources

    if (IS_ERR(pseudo_pdev)) {
        pr_err("pseudo_device: failed to register\n");
        return PTR_ERR(pseudo_pdev);
    }
    return 0;
}

/*
 * Module exit: unregister the platform device
 * (removes it from sysfs and unbinds from any driver).
 */
static void __exit pseudo_device_exit(void)
{
    pr_info("pseudo_device: exit\n");
    platform_device_unregister(pseudo_pdev);
}

module_init(pseudo_device_init);
module_exit(pseudo_device_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ragab");
MODULE_DESCRIPTION("Pseudo Platform Device");
