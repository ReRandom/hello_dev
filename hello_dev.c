#include <linux/init.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Roman Ponomarenko <r.e.p@yandex.ru>");

#define HELLO_VERSION "01"
#define DEVICE_NAME "hello_dev_device"
#define MODULE_NAME "hello_dev_module"
#define DEVICE_FIRST 0
#define DEVICE_COUNT 1
#define CLASS_NAME "hello_dev_class"

static int device_open(struct inode *inode, struct file *file);
static int device_release(struct inode *inode, struct file *file);
static ssize_t device_read(struct file *filp, char __user *buffer, size_t size,
				loff_t *offset);
static ssize_t device_write(struct file *filp, const char __user *buffer,
				size_t size, loff_t *offset);

static const struct file_operations hello_fops = {
.owner = THIS_MODULE,
.open = device_open,
.release = device_release,
.read = device_read,
.write = device_write,
};

static dev_t dev;
static struct cdev hcdev;
static struct class *devclass;
static int device_is_open;
static char *msg;
static char *msg_ptr;

static int __init hello_init(void)
{
	device_is_open = 0;
	int ret = alloc_chrdev_region(&dev, DEVICE_FIRST, DEVICE_COUNT,
					MODULE_NAME);
	if (ret < 0) {
		pr_err("Registering char device failed with %d\n",
				MAJOR(dev));
		return ret;
	}
	cdev_init(&hcdev, &hello_fops);
	hcdev.owner = THIS_MODULE;
	ret = cdev_add(&hcdev, dev, DEVICE_COUNT);
	if (ret < 0) {
		unregister_chrdev_region(dev, DEVICE_COUNT);
		pr_err("Can not add char device\n");
		return ret;
	}
	devclass = class_create(THIS_MODULE, CLASS_NAME);
	device_create(devclass, NULL, dev, "%s", DEVICE_NAME);
	msg = kmalloc(15, GFP_KERNEL);
	sprintf(msg, "Hello, world!\n");
	pr_info("[hello_dev v%s] init %d:%d\n", HELLO_VERSION,
			MAJOR(dev), MINOR(dev));
	return 0;
}

static void __exit hello_exit(void)
{
	device_destroy(devclass, dev);
	class_destroy(devclass);
	cdev_del(&hcdev);
	unregister_chrdev_region(dev, DEVICE_COUNT);
	kfree(msg);
	pr_info("[hello_dev v%s] exit\n", HELLO_VERSION);
}

static int device_open(struct inode *inode, struct file *file)
{
	if (device_is_open)
		return -EBUSY;
	++device_is_open;
	msg_ptr = msg;

	try_module_get(THIS_MODULE);

	pr_info("hello_dev v%s] open string: %s", HELLO_VERSION, msg);

	return 0;
}

static int device_release(struct inode *inode, struct file *file)
{
	--device_is_open;

	module_put(THIS_MODULE);

	pr_info("[hello_dev v%s] release", HELLO_VERSION);

	return 0;
}

static ssize_t device_read(struct file *filp, char __user *buffer, size_t size,
				loff_t *offset)
{
	ssize_t bytes_read = 0;

	if (*msg_ptr == 0) {
		msg_ptr = msg;
		return 0;
	}
	while (size && *msg_ptr) {
		put_user(*(msg_ptr++), buffer++);
		--size;
		++bytes_read;
	}
	pr_info("[hello_dev v%s] read size %ld from string: %s",
			HELLO_VERSION, size, msg);
	return bytes_read;
}

static ssize_t device_write(struct file *filp, const char __user *buffer,
				size_t size, loff_t *offset)
{
	ssize_t bytes_write = 0;

	pr_info("[hello_dev v%s] write %ld bytes", HELLO_VERSION,
		size);
	kfree(msg);
	msg = kmalloc(size+1, GFP_KERNEL);
	msg_ptr = msg;
	while (size) {
		get_user(*(msg_ptr++), buffer++);
		--size;
		++bytes_write;
	}
	if (*(msg_ptr-1) != 0)
		*msg_ptr = 0;
	msg_ptr = msg;
	return bytes_write;
}

module_init(hello_init);
module_exit(hello_exit);
