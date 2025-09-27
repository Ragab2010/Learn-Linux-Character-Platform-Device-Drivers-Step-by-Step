#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/ioport.h>

/* ------------------------------------------------------------------
 * Pseudo Device
 * ------------------------------------------------------------------ */

static struct resource pseudo_resources[] = {
    {
        .start = 0x10000000,
        .end   = 0x10000fff,
        .flags = IORESOURCE_MEM,
    },
    {
        .start = 42,   // fake IRQ number
        .end   = 42,
        .flags = IORESOURCE_IRQ,
    },
};

static struct platform_device *pseudo_pdev;

/* ------------------------------------------------------------------
 * Driver side
 * ------------------------------------------------------------------ */

static int pseudo_probe(struct platform_device *pdev)
{
    struct resource *res;

    pr_info("pseudo_driver: probe called for %s\n", dev_name(&pdev->dev));

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (res)
        pr_info("pseudo_driver: got MEM resource start=0x%lx, end=0x%lx\n",
                (unsigned long)res->start, (unsigned long)res->end);

    res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
    if (res)
        pr_info("pseudo_driver: got IRQ resource %lu\n",
                (unsigned long)res->start);

    return 0;
}

static int pseudo_remove(struct platform_device *pdev)
{
    pr_info("pseudo_driver: remove called for %s\n", dev_name(&pdev->dev));
    return 0;
}

static struct platform_driver pseudo_driver = {
    .probe  = pseudo_probe,
    .remove = pseudo_remove,
    .driver = {
        .name = "pseudo_device",  // must match pdev->name
        .owner = THIS_MODULE,
    },
};

/* ------------------------------------------------------------------
 * Init / Exit
 * ------------------------------------------------------------------ */

static int __init pseudo_init(void)
{
    int ret;

    pr_info("pseudo_module: init\n");

    /* Step 1: Register platform device */
    pseudo_pdev = platform_device_register_simple(
        "pseudo_device",   // must match driver name
        -1,
        pseudo_resources,
        ARRAY_SIZE(pseudo_resources));
    if (IS_ERR(pseudo_pdev)) {
        pr_err("pseudo_module: failed to register device\n");
        return PTR_ERR(pseudo_pdev);
    }

    /* Step 2: Register platform driver */
    ret = platform_driver_register(&pseudo_driver);
    if (ret) {
        pr_err("pseudo_module: failed to register driver\n");
        platform_device_unregister(pseudo_pdev);
        return ret;
    }

    return 0;
}

static void __exit pseudo_exit(void)
{
    pr_info("pseudo_module: exit\n");

    platform_driver_unregister(&pseudo_driver);
    platform_device_unregister(pseudo_pdev);
}

module_init(pseudo_init);
module_exit(pseudo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ragab");
MODULE_DESCRIPTION("Pseudo Platform Device + Driver Example");
