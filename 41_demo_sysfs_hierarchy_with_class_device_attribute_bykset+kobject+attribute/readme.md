Awesome 👍 Now let’s go **one level deeper** into the kernel’s object model.

The previous example used the **high-level device model** (`class` + `device` + `device_attribute`).
Now we’ll use the **raw object model** (`kset` + `kobject` + `attribute`).

---

# 🔹 What we’ll build

* `/sys/kernel/mykset`  → created from `struct kset`.
* `/sys/kernel/mykset/mykobj/` → a `struct kobject`.
* `/sys/kernel/mykse/mykobj/foo` → a custom `struct attribute`.

This avoids `class/device` and shows the **core sysfs machinery**.

---

# 🔹 Full Kernel Module (Raw Kobject + Kset)

```c
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
```

---

# 🔹 Build & Run

1. `make` with same Makefile as before.
2. Insert:

   ```bash
   sudo insmod mymodule.ko
   ```
3. Explore sysfs:

   ```bash
   ls /sys/kernel/mykset/mykobj/
   # should see: foo
   ```
4. Test:

   ```bash
   cat /sys/kernel/mykset/mykobj/foo
   echo 77 | sudo tee /sys/kernel/mykset/mykobj/foo
   cat /sys/kernel/mykset/mykobj/foo
   ```
5. Remove:

   ```bash
   sudo rmmod mymodule
   ```

---

# 🔹 Key Differences from `class/device` version

| Aspect               | `class/device` API                                         | Raw `kobject/kset` API                              |
| -------------------- | ---------------------------------------------------------- | --------------------------------------------------- |
| Directory `/sys/kernel/...` | `/sys/class/...` or `/sys/devices/...`                     | Any name you choose (`/sys/kernel/mykset/...`)             |
| Struct used          | `struct class`, `struct device`, `struct device_attribute` | `struct kset`, `struct kobject`, `struct attribute` |
| Convenience          | High-level helpers, auto cleanup                           | Low-level, manual management                        |
| Use cases            | Drivers, devices, subsystems                               | Core kernel objects, custom subsystems              |

---

✅ So now you’ve seen both:

* The **high-level device model** (`class` + `device` + `device_attribute`).
* The **raw kobject model** (`kset` + `kobject` + `attribute`).

---

👉 Do you want me to draw a **combined diagram** showing where both approaches fit in the `/sys` hierarchy (so you can compare `class/device` vs. raw `kset/kobject` side by side)?
