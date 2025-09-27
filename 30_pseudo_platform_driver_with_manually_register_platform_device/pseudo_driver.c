// pseudo_driver.c
#include <linux/module.h>
#include <linux/platform_device.h>

/*
 * Probe: called automatically by the kernel when a registered driver
 * matches a registered platform_device by name (or Device Tree match).
 *
 * Here we retrieve the resources (MMIO, IRQ) and print them.
 */
static int pseudo_probe(struct platform_device *pdev)
{
    struct resource *res;

    pr_info("pseudo_driver: probe called for %s\n", dev_name(&pdev->dev));

    // Get the first MEM resource (index 0)
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (res)
        pr_info("pseudo_driver: got MEM resource start=0x%lx end=0x%lx\n",
                (unsigned long)res->start, (unsigned long)res->end);

    // Get the first IRQ resource (index 0)
    res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
    if (res)
        pr_info("pseudo_driver: got IRQ resource %lu\n",
                (unsigned long)res->start);

    return 0; // success
}

/*
 * Remove: called when the device is unbound from the driver
 * (either driver is removed or device is unregistered).
 */
static int pseudo_remove(struct platform_device *pdev)
{
    pr_info("pseudo_driver: remove called for %s\n", dev_name(&pdev->dev));
    return 0;
}

/*
 * The platform_driver structure tells the kernel about our driver:
 *   - probe/remove callbacks
 *   - driver name (must match the platform_device's name!)
 *   - owner (module reference)
 */
static struct platform_driver pseudo_driver = {
    .probe  = pseudo_probe,
    .remove = pseudo_remove,
    .driver = {
        .name  = "pseudo_device",  // must match device name
        .owner = THIS_MODULE,
    },
};

/*
 * module_platform_driver() is a convenience macro:
 *   - Creates init/exit functions for registering/unregistering driver.
 *   - Inside init: calls platform_driver_register(&pseudo_driver).
 *   - Inside exit: calls platform_driver_unregister(&pseudo_driver).
 */
module_platform_driver(pseudo_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ragab");
MODULE_DESCRIPTION("Pseudo Platform Driver");
