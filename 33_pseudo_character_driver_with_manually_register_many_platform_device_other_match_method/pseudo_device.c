#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include "pseudo_common.h" 

/* 
 * Each pseudo device will carry some custom configuration data
 * in "platform_data". This is typically filled in by board code
 * or device tree in real hardware scenarios.
 */
// struct pseudo_platform_data {
// 	int some_value;     // arbitrary configuration parameter
// 	char label[20];     // label string to identify device
// };

/* 
 * Global pointers to our pseudo devices. 
 * We create 3 devices dynamically.
 */
static struct platform_device *pdev1, *pdev2, *pdev3;

/* 
 * Module init: allocate and register 3 pseudo devices.
 * Devices are registered under the name "pseudo-char".
 * That name must match the driver’s id_table for a successful probe.
 */
static int __init pseudo_device_init(void)
{
	struct pseudo_platform_data *pdata;

	/* -------- Device 1 -------- */
	pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	pdata->some_value = 111;
	strscpy(pdata->label, "Device_One", sizeof(pdata->label));

	/* Create a platform_device with name "pseudo-char" and ID = 1 */
	pdev1 = platform_device_alloc("pseudo-char", 1);
	if (!pdev1) {
		kfree(pdata);
		return -ENOMEM;
	}

	/* Attach platform_data (our config struct) to the device */
	pdev1->dev.platform_data = pdata;

	/* Register the device with the kernel */
	platform_device_add(pdev1);

	/* -------- Device 2 -------- */
	pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	pdata->some_value = 222;
	strscpy(pdata->label, "Device_Two", sizeof(pdata->label));

	pdev2 = platform_device_alloc("pseudo-char", 2);
	if (!pdev2) {
		kfree(pdata);
		return -ENOMEM;
	}
	pdev2->dev.platform_data = pdata;
	platform_device_add(pdev2);

	/* -------- Device 3 -------- */
	pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	pdata->some_value = 333;
	strscpy(pdata->label, "Device_Three", sizeof(pdata->label));

	pdev3 = platform_device_alloc("pseudo-char", 3);
	if (!pdev3) {
		kfree(pdata);
		return -ENOMEM;
	}
	pdev3->dev.platform_data = pdata;
	platform_device_add(pdev3);

	pr_info("Pseudo devices registered successfully\n");
	return 0;
}

/* 
 * Module exit: cleanup.
 * For each device:
 *   - unregister from kernel
 *   - free its platform_data
 */
static void __exit pseudo_device_exit(void)
{
    struct pseudo_platform_data *pdata;

    if (pdev1) {
        pdata = pdev1->dev.platform_data;
        pdev1->dev.platform_data = NULL;    /* detach to avoid double-free */
        platform_device_unregister(pdev1);
        kfree(pdata);
    }

    if (pdev2) {
        pdata = pdev2->dev.platform_data;
        pdev2->dev.platform_data = NULL;
        platform_device_unregister(pdev2);
        kfree(pdata);
    }

    if (pdev3) {
        pdata = pdev3->dev.platform_data;
        pdev3->dev.platform_data = NULL;
        platform_device_unregister(pdev3);
        kfree(pdata);
    }

    pr_info("Pseudo devices unregistered\n");
}

module_init(pseudo_device_init);
module_exit(pseudo_device_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ragab Example");
MODULE_DESCRIPTION("Pseudo Platform Devices (3 devices)");
