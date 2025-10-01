#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("YourName");
MODULE_DESCRIPTION("Raw kobject + kset sysfs demo");
MODULE_VERSION("1.0");

static struct kset *example_kset;
static struct kobject *example_kobj;

static int foo_value = 0;

/* show callback for attribute */
static ssize_t foo_show(struct kobject *kobj,
                        struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", foo_value);
}

/* store callback for attribute */
static ssize_t foo_store(struct kobject *kobj,
                         struct kobj_attribute *attr,
                         const char *buf, size_t count)
{
    int temp;
    if (sscanf(buf, "%d", &temp) == 1)
        foo_value = temp;
    return count;
}

/* define attribute */
static struct kobj_attribute foo_attr =
    __ATTR(foo, 0664, foo_show, foo_store);

static int __init mymodule_init(void)
{
    int ret;

    /* 1. Create a kset (appears as /sys/kernel/mykset) */
    example_kset = kset_create_and_add("mykset", NULL, kernel_kobj);
    if (!example_kset)
        return -ENOMEM;

    /* 2. Create a kobject under that kset (appears as /sys/kernel/mykset/mykobj) */
    example_kobj = kobject_create_and_add("mykobj", &example_kset->kobj);
    if (!example_kobj) {
        kset_unregister(example_kset);
        return -ENOMEM;
    }

    /* 3. Add attribute file (/sys/kernel/mykset/mykobj/foo) */
    ret = sysfs_create_file(example_kobj, &foo_attr.attr);
    if (ret) {
        kobject_put(example_kobj);
        kset_unregister(example_kset);
        return ret;
    }

    pr_info("mymodule(raw): loaded\n");
    return 0;
}

static void __exit mymodule_exit(void)
{
    sysfs_remove_file(example_kobj, &foo_attr.attr);
    kobject_put(example_kobj);
    kset_unregister(example_kset);
    pr_info("mymodule(raw): unloaded\n");
}

module_init(mymodule_init);
module_exit(mymodule_exit);
