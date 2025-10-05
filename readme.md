
# 🧠 Learn Linux Character & Platform Device Drivers – Step by Step

### Author: **Ragab Elsayed**

📍 Embedded Linux & Kernel Developer — Egypt

---

## 🎯 Purpose

This repository is a **progressive, hands-on learning path** that helps you **build deep understanding of Linux device driver development** — starting from basic **character drivers** and gradually advancing to **platform drivers**, **device tree integration**, **sysfs attributes**, and **real hardware interaction** (e.g., GPIO, I2C, LEDs on Raspberry Pi 4B).

It’s designed to teach **concepts incrementally**, showing how kernel subsystems interact:
`cdev`, `class`, `device`, `platform_device`, `platform_driver`, `sysfs`, `DT overlay`, and more.

---

## 🧩 Learning Path Overview

The project evolves gradually through **real, working driver examples**:

### **Stage 1 — Core Character Device Driver Basics**
| Directory | Focus | Description |
|------------|--------|-------------|
| `1_major_minor` | Major/Minor numbers | Learn how Linux identifies devices using `dev_t`, `MAJOR()`, `MINOR()`. |
| `2_static_allocate_major_minor` | Static registration | Manually register device numbers using `register_chrdev_region()`. |
| `3_dynamic_allocate_major_minor` | Dynamic allocation | Use `alloc_chrdev_region()` to dynamically allocate a major/minor. |
| `4_create_class` | Creating `class` | Create a `class` entry under `/sys/class`. |
| `5_automatic_create_device_note` | Auto-create `/dev` node | Use `device_create()` to automatically create device nodes. |
| `6_automatic_create_deviceNode_with_dynamic_deviceNumber` | Combine class + dynamic number | Full dynamic character device with auto node creation. |

---

### **Stage 2 — File Operations (fops) & User Space Communication**
| Directory | Focus | Description |
|------------|--------|-------------|
| `7_cdev_by_dynamic_allocation` | `cdev` interface | Register and manage devices with `struct cdev`. |
| `9_copy_from_user_and_copy_to_user` | User space I/O | Safely copy data between user and kernel space. |
| `10_copy_from_user_and_copy_to_user_struct_version` | Struct exchange | Exchange structures between user and kernel space. |
| `13_copy_from_user_and_copy_to_user_struct_version_with_heap_member` | Heap members | Handle structs containing pointers dynamically. |
| `15_write_read_lseek` | File ops | Implement `write`, `read`, and `lseek` operations. |
| `18_multiple_device_nodes_private_data` | Private data | Maintain per-device private structures (`file->private_data`). |

---

### **Stage 3 — Advanced File Control (ioctl, permissions, capability checks)**
| Directory | Focus | Description |
|------------|--------|-------------|
| `19_single_device_ioctl_fops` | Simple IOCTL | Implement IOCTL commands for configuration. |
| `21_single_device_ioctl_fops_complex_cmd` | Complex IOCTL | Handle structs and command IDs safely. |
| `24_single_device_open_once_at_time` | Access control | Restrict simultaneous device access. |
| `26_single_device_add_userapp_CAP_DAC_OVERRIDE` | Capabilities | Test with Linux capabilities (`CAP_DAC_OVERRIDE`). |
| `27_single_device_add_ioctl_capable` | Capability checks in IOCTL | Ensure safe privileged commands. |

---

### **Stage 4 — Multiple Devices & Platform Devices**
| Directory | Focus | Description |
|------------|--------|-------------|
| `30_pseudo_platform_driver_with_manually_register_platform_device` | Intro to platform devices | Manually register a platform device and driver. |
| `31_pseudo_character_driver_with_manually_register_platform_device` | Add `cdev` | Character driver integrated with a platform device. |
| `32_pseudo_character_driver_with_manually_register_many_Platform_device` | Multiple devices | Register several platform devices manually. |
| `33_pseudo_character_driver_with_manually_register_many_platform_device_other_match_method` | Matching | Use `struct platform_device_id` for matching. |

---

### **Stage 5 — Device Tree (DT) Integration**
| Directory | Focus | Description |
|------------|--------|-------------|
| `34_pseudo_driver_with_device_tree_node_as_device` | Match with DT node | Bind platform driver to DT node. |
| `35_pseudo_driver_with_multiple_devicefile_from_device_tree_nodes` | Multi-device from DT | Create `/dev/pseudoX` for each DT node. |
| `36_pseudo_driver_with_from_device_tree_with_device_tree_overlay` | DT overlays | Dynamically load new devices via overlay. |

📄 Each of these folders includes:
- A `.dts` or overlay source file.
- A driver source file implementing `of_match_table`.
- Flow diagram Markdown files (`the_flow_from_deviceTree_to_driver.md`) explaining binding steps.

---

### **Stage 6 — Subsystems, Sysfs, and Attributes**
| Directory | Focus | Description |
|------------|--------|-------------|
| `37_subsystem_demonstrating_kset-kobject-ktype-attributes_by_struct_attribute` | Core object model | Learn `kset`, `kobject`, and `ktype` relations. |
| `38_subsystem_demonstrating_kset-kobject-ktype-attributes_by_struct_device_attribute` | Device attributes | Expose attributes via `struct device_attribute`. |
| `39_demo_sysfs_hierarchy_with_class` | Sysfs hierarchy | Build `/sys/class/myclass/mydevice/`. |
| `40_demo_sysfs_hierarchy_with_class_device_attribute` | Add attributes | Expose readable and writable sysfs attributes. |
| `41_demo_sysfs_hierarchy_with_class_device_attribute_bykset+kobject+attribute` | Mix hierarchy | Combine class + kobject + attributes cleanly. |

---

### **Stage 7 — Raspberry Pi 4B Hardware-Oriented Drivers**
| Directory | Focus | Description |
|------------|--------|-------------|
| `42_rappi4b_simple_LED_consumer_driver_match_DT_node_expose_brightness_attribute_at_sysfs` | Simple LED driver | Match DT node, expose `brightness` in sysfs. |
| `43_rappi4b_simple_LED_consumer_driver_match_DT_node_expose_deviceFile_in_dev_diroctry` | Add device node | `/dev/myled` + DT matching. |
| `44_rappi4b_simple_LED_consumer_driver_match_DT_node_expose_deviceFile_and_attributes` | Combined sysfs + device file | Complete LED consumer driver. |
| `45_rappi4b_simple_i2cDevice_consumer_driver_match_DT_node_pinconfigration_expose_attributes` | I²C example | Demonstrates pinmux and DT binding. |
| `46_rappi4b_simple_pseudo_LED_consumer_driver_expose_attribute_at_sysfs_and_deviceFile_with_mutex` | Safe concurrency | Protect shared resources using `mutex`. |
| `47_rappi4b_simple_LED__gpio_consumer_driver_expose_attribute_at_sysfs_and_deviceFile_with_mutex` | GPIO consumer | LED GPIO control using DT node pinmux. |

---

## 🧠 What You’ll Learn

By completing this repository step-by-step, you’ll deeply understand:

- ✅ How Linux handles **character devices** (`cdev`, `fops`, major/minor).
- ✅ How **user space and kernel space communicate**.
- ✅ How to safely use **`copy_to_user()` / `copy_from_user()`**.
- ✅ How to handle **ioctl commands**, access control, and concurrency.
- ✅ How to write **Platform Drivers** and register **Platform Devices**.
- ✅ How **Device Tree nodes** match drivers using `compatible` strings.
- ✅ How **Device Tree Overlays** dynamically create devices at runtime.
- ✅ How to expose **attributes via sysfs** and **device files in /dev**.
- ✅ How to integrate with real **Raspberry Pi 4 hardware**, including GPIO and I²C devices.

---

## 🛠️ Build & Run Example

Each subdirectory contains a `Makefile` that builds the module:
```bash
make
sudo insmod <module_name>.ko
````

To remove:

```bash
sudo rmmod <module_name>
```

For user applications (if present):

```bash
cd userapp
gcc userapp.c -o userapp
./userapp
```

---

## 🧩 Example: Testing Platform Driver with Device Tree Overlay

```bash
# Compile device tree overlay
dtc -@ -I dts -O dtb -o pseudo-overlay.dtbo pseudo-overlay.dts

# Apply overlay at runtime
sudo dtoverlay pseudo-overlay.dtbo

# Load the driver
sudo insmod pseudo_driver.ko

# Check logs
dmesg | tail
```

---

## 📚 Visual Learning Aids

Several modules (e.g. 35, 36, 41) include Markdown diagrams showing:

* How **Device Tree → Platform Device → Driver Probe** flow works.
* How **overlay injection** dynamically creates new devices.
* How `/sys`, `/dev`, and `/proc` entries are related.

---

## 💡 Tips for Learners

* Start from folder `1_...` and move sequentially.
* Read all inline comments — they are **heavily documented** for clarity.
* After building, explore `/sys/class/`, `/sys/devices/platform/`, and `/dev/` to observe kernel object creation.
* Always inspect logs:

  ```bash
  dmesg | tail -n 30
  ```

---

## 📖 License

MIT License — free for educational and research use.

---

## 👨‍💻 Author

**Ragab Elsayed**
Embedded Linux & C/C++ Software Engineer
📍 Egypt
📧 [ragabelsayedd@gmail.com](mailto:ragabelsayedd@gmail.com)
🔗 [LinkedIn](https://bit.ly/3CX6czL) | [GitHub](https://github.com/ragab2010) | [YouTube](https://bit.ly/2oVoSiz)

---

> *“The best way to learn Linux drivers is not by reading — it’s by **building one layer at a time**, and this repository does exactly that.”*

```

---