#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/mod_devicetable.h>
#include "pseudo_common.h" 

/* 
 * This struct represents "driver-private data" stored per device instance.
 * Think of it as runtime state the driver maintains for each probed device.
 */
struct pseudo_driver_data {
	int device_index;   // index provided by id_table->driver_data
};

/* 
 * Match table for this driver.
 * - The "name" field must match the device->name ("pseudo-char").
 * - The "driver_data" field is an arbitrary index/config value.
 *
 * Here we define 3 entries for our 3 pseudo devices.
 */
static const struct platform_device_id pseudo_id_table[] = {
	{ "pseudo-char", 1 },  // device index 1
	{ "pseudo-char", 2 },  // device index 2
	{ "pseudo-char", 3 },  // device index 3
	{ } /* sentinel to mark end of table */
};
MODULE_DEVICE_TABLE(platform, pseudo_id_table);

/* 
 * Probe function: called by kernel when a matching device is found.
 * Steps:
 *   1. Identify matching entry in id_table
 *   2. Retrieve platform_data (supplied by pseudo_device.c)
 *   3. Allocate driver_data (runtime storage for this device)
 *   4. Store driver_data with platform_set_drvdata()
 */
static int pseudo_probe(struct platform_device *pdev)
{
	const struct platform_device_id *id;
	struct pseudo_platform_data *pdata;
	struct pseudo_driver_data *drvdata;

	/* Find match in id_table */
	id = platform_get_device_id(pdev);
	if (!id)
		return -ENODEV;

	/* Get platform_data pointer from device */
	pdata = dev_get_platdata(&pdev->dev);
	if (!pdata) {
		dev_err(&pdev->dev, "No platform_data found!\n");
		return -EINVAL;
	}

	/* Allocate memory for driver_data */
	drvdata = devm_kzalloc(&pdev->dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	/* Fill driver_data using id_table->driver_data */
	drvdata->device_index = id->driver_data;

	/* Attach driver_data to device */
	platform_set_drvdata(pdev, drvdata);

	dev_info(&pdev->dev,
		 "Probed: name=%s, id=%d, pdata->value=%d, pdata->label=%s, drv_index=%d\n",
		 id->name, pdev->id,
		 pdata->some_value, pdata->label,
		 drvdata->device_index);

	return 0;
}

/* 
 * Remove function: called when device is unbound/unregistered.
 * Used to cleanup driver_data or hardware resources.
 */
static int pseudo_remove(struct platform_device *pdev)
{
	struct pseudo_driver_data *drvdata = platform_get_drvdata(pdev);

	dev_info(&pdev->dev, "Removed device index=%d\n", drvdata->device_index);

	return 0;
}

/* 
 * Platform driver structure.
 * - driver.name must match device name ("pseudo-char").
 * - id_table is required for name-based matching.
 */
static struct platform_driver pseudo_driver = {
	.probe = pseudo_probe,
	.remove = pseudo_remove,
	.driver = {
		.name = "pseudo-char",
		.owner = THIS_MODULE,
	},
	.id_table = pseudo_id_table,
};

/* Register driver with kernel */
module_platform_driver(pseudo_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ragab Example");
MODULE_DESCRIPTION("Pseudo Character Platform Driver with id_table");
