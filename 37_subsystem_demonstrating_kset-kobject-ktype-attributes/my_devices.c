// my_devices.c - simple kset/kobject/ktype example
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/slab.h>
#include <linux/sysfs.h>
#include <linux/string.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Example Author");
MODULE_DESCRIPTION("Toy subsystem demonstrating kset -> kobject -> ktype -> attributes");
MODULE_VERSION("0.1");

struct my_dev {
    struct kobject kobj;
    int value;
    char status[16];
};

/* forward declarations */
static ssize_t my_show(struct kobject *kobj, struct attribute *attr, char *buf);
static ssize_t my_store(struct kobject *kobj, struct attribute *attr,
                        const char *buf, size_t count);
static void my_release(struct kobject *kobj);

/* sysfs ops */
static const struct sysfs_ops my_sysfs_ops = {
    .show  = my_show,
    .store = my_store,
};

/* attributes: status (ro), value (rw) */
static struct attribute attr_status = {
    .name = "status",
    .mode = 0444,
};
static struct attribute attr_value = {
    .name = "value",
    .mode = 0644,
};

static struct attribute *my_default_attrs[] = {
    &attr_status,
    &attr_value,
    NULL,
};

static struct kobj_type my_ktype = {
    .release = my_release,
    .sysfs_ops = &my_sysfs_ops,
    .default_attrs = my_default_attrs,
};

/* kset pointer */
static struct kset *my_kset;

/* keep pointers to created devices so we can put them at module exit */
static struct my_dev *dev1;
static struct my_dev *dev2;

/* ---------- helper: container_of to map kobject -> my_dev ---------- */
static inline struct my_dev *to_my_dev(struct kobject *kobj)
{
    return container_of(kobj, struct my_dev, kobj);
}

/* ---------- sysfs show/store implementations ---------- */
static ssize_t my_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
    struct my_dev *mdev = to_my_dev(kobj);

    if (strcmp(attr->name, "status") == 0) {
        return scnprintf(buf, PAGE_SIZE, "%s\n", mdev->status);
    } else if (strcmp(attr->name, "value") == 0) {
        return scnprintf(buf, PAGE_SIZE, "%d\n", mdev->value);
    }

    /* unknown attribute */
    return -EIO;
}

static ssize_t my_store(struct kobject *kobj, struct attribute *attr,
                        const char *buf, size_t count)
{
    struct my_dev *mdev = to_my_dev(kobj);
    long v;
    int ret;

    if (strcmp(attr->name, "value") == 0) {
        ret = kstrtol(buf, 0, &v);
        if (ret)
            return ret;
        mdev->value = (int)v;
        return count;
    }

    /* read-only or unknown attribute */
    return -EIO;
}

/* ---------- release ---------- */
static void my_release(struct kobject *kobj)
{
    struct my_dev *mdev = to_my_dev(kobj);
    pr_info("my_devices: releasing %s\n", kobject_name(kobj));
    kfree(mdev);
}

/* ---------- module init/exit ---------- */
static int __init my_module_init(void)
{
    int ret;

    pr_info("my_devices: init\n");

    /* create a kset under /sys/kernel by passing kernel_kobj as parent
     * It will create: /sys/kernel/my_devices
     * If you want it under /sys/class instead, use kset_create_and_add("my_devices", NULL, NULL)
     * and then create under class root; for simplicity we add under kernel_kobj.
     */
    my_kset = kset_create_and_add("my_devices", NULL, kernel_kobj);
    if (!my_kset) {
        pr_err("my_devices: failed to create kset\n");
        return -ENOMEM;
    }

    /* allocate and initialize dev1 */
    dev1 = kzalloc(sizeof(*dev1), GFP_KERNEL);
    if (!dev1) {
        ret = -ENOMEM;
        goto err_free_kset;
    }
    strncpy(dev1->status, "OK", sizeof(dev1->status));
    dev1->value = 1;

    ret = kobject_init_and_add(&dev1->kobj, &my_ktype, &my_kset->kobj, "dev1");
    if (ret) {
        pr_err("my_devices: failed to add dev1 kobject: %d\n", ret);
        kobject_put(&dev1->kobj); /* will call release if refcount 0 */
        dev1 = NULL;
        goto err_free_kset;
    }
    kobject_uevent(&dev1->kobj, KOBJ_ADD);

    /* allocate and initialize dev2 */
    dev2 = kzalloc(sizeof(*dev2), GFP_KERNEL);
    if (!dev2) {
        ret = -ENOMEM;
        goto err_put_dev1;
    }
    strncpy(dev2->status, "OK", sizeof(dev2->status));
    dev2->value = 42;

    ret = kobject_init_and_add(&dev2->kobj, &my_ktype, &my_kset->kobj, "dev2");
    if (ret) {
        pr_err("my_devices: failed to add dev2 kobject: %d\n", ret);
        kobject_put(&dev2->kobj);
        dev2 = NULL;
        goto err_put_dev1;
    }
    kobject_uevent(&dev2->kobj, KOBJ_ADD);

    pr_info("my_devices: created /sys/kernel/my_devices/dev1 and dev2\n");
    return 0;

err_put_dev1:
    if (dev1) {
        kobject_put(&dev1->kobj);
        dev1 = NULL;
    }
err_free_kset:
    kset_unregister(my_kset);
    my_kset = NULL;
    return ret;
}

static void __exit my_module_exit(void)
{
    pr_info("my_devices: exit\n");

    /* remove dev1 and dev2 */
    if (dev2) {
        kobject_uevent(&dev2->kobj, KOBJ_REMOVE);
        kobject_put(&dev2->kobj);
        dev2 = NULL;
    }
    if (dev1) {
        kobject_uevent(&dev1->kobj, KOBJ_REMOVE);
        kobject_put(&dev1->kobj);
        dev1 = NULL;
    }

    /* unregister kset (also removes kset kobject) */
    if (my_kset) {
        kset_unregister(my_kset);
        my_kset = NULL;
    }

    pr_info("my_devices: cleaned up\n");
}

module_init(my_module_init);
module_exit(my_module_exit);
