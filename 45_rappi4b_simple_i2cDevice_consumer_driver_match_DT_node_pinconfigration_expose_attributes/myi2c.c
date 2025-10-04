#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <linux/slab.h>

/* Private device data */
struct myi2cdev_data {
    struct i2c_client *client;
    u8 value;   /* fake register value */
};

/* Sysfs show/store for "value" */
static ssize_t value_show(struct device *dev,
                          struct device_attribute *attr, char *buf)
{
    struct myi2cdev_data *data = dev_get_drvdata(dev);
    return sprintf(buf, "%u\n", data->value);
}

static ssize_t value_store(struct device *dev,
                           struct device_attribute *attr,
                           const char *buf, size_t count)
{
    struct myi2cdev_data *data = dev_get_drvdata(dev);
    int ret;
    u8 val;

    ret = kstrtou8(buf, 0, &val);
    if (ret)
        return ret;

    data->value = val;

    /* here you would normally talk to the device via i2c_smbus_write_byte_data */
    dev_info(dev, "New value written: %u\n", data->value);

    return count;
}

static DEVICE_ATTR_RW(value);

/* Sysfs show for "info" (read-only) */
static ssize_t info_show(struct device *dev,
                         struct device_attribute *attr, char *buf)
{
    struct myi2cdev_data *data = dev_get_drvdata(dev);

    return sprintf(buf,
        "Fake I2C device @ addr 0x%02x, last value=%u\n",
        data->client->addr, data->value);
}

static DEVICE_ATTR_RO(info);

static struct attribute *myi2cdev_attrs[] = {
    &dev_attr_value.attr,
    &dev_attr_info.attr,
    NULL,
};

static const struct attribute_group myi2cdev_attr_group = {
    .attrs = myi2cdev_attrs,
};

/* Probe/remove */
static int myi2cdev_probe(struct i2c_client *client,
                          const struct i2c_device_id *id)
{
    struct myi2cdev_data *data;
    int ret;

    dev_info(&client->dev, "Probing myi2cdev at addr 0x%02x\n", client->addr);

    data = devm_kzalloc(&client->dev, sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    data->client = client;
    data->value = 0;

    i2c_set_clientdata(client, data);

    /* Create sysfs files */
    ret = sysfs_create_group(&client->dev.kobj, &myi2cdev_attr_group);
    if (ret)
        return ret;

    dev_info(&client->dev, "myi2cdev initialized successfully\n");
    return 0;
}

static int myi2cdev_remove(struct i2c_client *client)
{
    sysfs_remove_group(&client->dev.kobj, &myi2cdev_attr_group);
    dev_info(&client->dev, "myi2cdev removed\n");
    return 0;
}

/* Match table for OF */
static const struct of_device_id myi2cdev_of_match[] = {
    { .compatible = "ragab,myi2cdev" },
    { }
};
MODULE_DEVICE_TABLE(of, myi2cdev_of_match);

/* ID table (for non-DT systems) */
static const struct i2c_device_id myi2cdev_id[] = {
    { "myi2cdev", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, myi2cdev_id);

/* Driver struct */
static struct i2c_driver myi2cdev_driver = {
    .driver = {
        .name = "myi2cdev",
        .of_match_table = myi2cdev_of_match,
    },
    .probe = myi2cdev_probe,
    .remove = myi2cdev_remove,
    .id_table = myi2cdev_id,
};

module_i2c_driver(myi2cdev_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ragab");
MODULE_DESCRIPTION("Fake I2C device driver with sysfs attributes");
