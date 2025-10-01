Perfect 🚀 Let’s build a **hands-on kernel module** that demonstrates the full hierarchy:

We’ll create:

* a **class** → `/sys/class/myclass/`
* a **device** under it → `/sys/class/myclass/mydev/`
* an **attribute** file → `/sys/class/myclass/mydev/value`

You’ll be able to `cat`/`echo` into it from user space.

---

## 🔹 Full Example Module

```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("YourName");
MODULE_DESCRIPTION("Demo sysfs hierarchy with class/device/attribute");
MODULE_VERSION("1.0");

static struct class *my_class;
static struct device *my_device;

static int value = 0;

/* show callback → triggered on cat */
static ssize_t value_show(struct device *dev,
                          struct device_attribute *attr,
                          char *buf)
{
    return sprintf(buf, "%d\n", value);
}

/* store callback → triggered on echo */
static ssize_t value_store(struct device *dev,
                           struct device_attribute *attr,
                           const char *buf, size_t count)
{
    int temp;
    if (sscanf(buf, "%d", &temp) == 1)
        value = temp;
    return count;
}

/* define device attribute (rw) */
static DEVICE_ATTR(value, 0664, value_show, value_store);

static int __init mymodule_init(void)
{
    int ret;

    /* 1. Create class (/sys/class/myclass) */
    my_class = class_create(THIS_MODULE, "myclass");
    if (IS_ERR(my_class))
        return PTR_ERR(my_class);

    /* 2. Create device (/sys/class/myclass/mydev) */
    my_device = device_create(my_class, NULL, 0, NULL, "mydev");
    if (IS_ERR(my_device)) {
        class_destroy(my_class);
        return PTR_ERR(my_device);
    }

    /* 3. Create attribute file (/sys/class/myclass/mydev/value) */
    ret = device_create_file(my_device, &dev_attr_value);
    if (ret) {
        device_destroy(my_class, 0);
        class_destroy(my_class);
        return ret;
    }

    pr_info("mymodule: loaded\n");
    return 0;
}

static void __exit mymodule_exit(void)
{
    device_remove_file(my_device, &dev_attr_value);
    device_destroy(my_class, 0);
    class_destroy(my_class);
    pr_info("mymodule: unloaded\n");
}

module_init(mymodule_init);
module_exit(mymodule_exit);
```

---

## 🔹 Build instructions

1. Save as `mymodule.c`.

2. Create `Makefile`:

   ```make
   obj-m += mymodule.o

   all:
   	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

   clean:
   	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
   ```

3. Build:

   ```bash
   make
   sudo insmod mymodule.ko
   ```

4. Check in sysfs:

   ```bash
   ls /sys/class/myclass/mydev/
   # should see: power  subsystem  uevent  value
   ```

5. Test attribute:

   ```bash
   cat /sys/class/myclass/mydev/value
   echo 123 > /sys/class/myclass/mydev/value
   cat /sys/class/myclass/mydev/value
   ```

6. Remove:

   ```bash
   sudo rmmod mymodule
   ```

---

✅ This demonstrates the **whole sysfs hierarchy**:

* `struct class` → `/sys/class/myclass/`
* `struct device` → `/sys/class/myclass/mydev/`
* `struct device_attribute` → `/sys/class/myclass/mydev/value`

---

Would you like me to also extend this example with a **`kset` + raw `kobject` + `struct attribute` (not device_attribute)** version, so you see the *lower-level API* as well?
