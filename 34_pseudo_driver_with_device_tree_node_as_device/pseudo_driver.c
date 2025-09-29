#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>          /* For DT support */
#include <linux/slab.h>

struct pseudo_platform_data {
    int value;
    const char *label;
};

struct pseudo_driver_data {
    int device_index;
    struct pseudo_platform_data pdata;
};

/* --- Device Tree match table --- */
static const struct of_device_id pseudo_of_match[] = {
    { .compatible = "mycompany,pseudo-char", .data = (void *)1 },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, pseudo_of_match);

/* Probe function */
static int pseudo_probe(struct platform_device *pdev)
{
    struct pseudo_driver_data *drvdata;
    const struct of_device_id *match;
    struct device_node *np = pdev->dev.of_node;

    if (!np)
        return -EINVAL;

    drvdata = devm_kzalloc(&pdev->dev, sizeof(*drvdata), GFP_KERNEL);
    if (!drvdata)
        return -ENOMEM;

    /* Match entry */
    match = of_match_node(pseudo_of_match, np);
    if (match)
        drvdata->device_index = (long)match->data;  // comes from .data in table

    /* Read DT properties */
    of_property_read_u32(np, "value", &drvdata->pdata.value);
    of_property_read_string(np, "label", &drvdata->pdata.label);

    dev_set_drvdata(&pdev->dev, drvdata);

    dev_info(&pdev->dev, "Probed: index=%d, value=%d, label=%s\n",
             drvdata->device_index, drvdata->pdata.value,
             drvdata->pdata.label ? drvdata->pdata.label : "NULL");

    return 0;
}

/* Remove function */
static int pseudo_remove(struct platform_device *pdev)
{
    struct pseudo_driver_data *drvdata = dev_get_drvdata(&pdev->dev);

    dev_info(&pdev->dev, "Removed device index=%d\n", drvdata->device_index);
    return 0;
}

/* Platform driver */
static struct platform_driver pseudo_driver = {
    .probe  = pseudo_probe,
    .remove = pseudo_remove,
    .driver = {
        .name           = "pseudo-char-dt",
        .of_match_table = pseudo_of_match,
    },
};

module_platform_driver(pseudo_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ragab Elsayed");
MODULE_DESCRIPTION("Pseudo char platform driver using Device Tree");
