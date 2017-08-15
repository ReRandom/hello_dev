#include <linux/init.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_LICENSE("GPLv2");
MODULE_AUTHOR("Roman Ponomarenko <r.e.p@yandex.ru>");

#define HELLO_VERSION "00"
#define DEVICE_NAME "hello_dev"

static int device_open(struct inode* inode, struct file* file);
static int device_release(struct inode* inode, struct file* file);
static ssize_t device_read(struct file* filp, char __user* buffer, size_t size, loff_t* offset);
static ssize_t device_write(struct file* filp, const char __user* buffer, size_t size, loff_t* offset);

static struct file_operations hello_fops = {
.owner = THIS_MODULE,
.open = device_open,
.release = device_release,
.read = device_read,
.write = device_write,
};

static int Major;
static int device_is_open = 0;
static char *msg;
static char *msg_ptr;

static int __init hello_init(void)
{
	Major = register_chrdev(0, DEVICE_NAME, &hello_fops);
	if(Major < 0)
	{
		printk(KERN_ALERT "Registering char device failed with %d\n", Major);
		return Major;
	}
	msg = kmalloc(15, GFP_KERNEL);
	sprintf(msg, "Hello, world!\n");
	printk(KERN_INFO "[hello_dev v%s] init\n", HELLO_VERSION);
	return 0;
}

static void __exit hello_exit(void)
{
	unregister_chrdev(Major, DEVICE_NAME);
	kfree(msg);
	printk(KERN_INFO "[hello_dev v%s] exit\n", HELLO_VERSION);
}

static int device_open(struct inode* inode, struct file* file)
{
	if(device_is_open)
		return -EBUSY;
	++device_is_open;
	msg_ptr = msg;

	try_module_get(THIS_MODULE);

	printk(KERN_INFO "[hello_dev v%s] open string: %s", HELLO_VERSION, msg);

	return 0;
}

static int device_release(struct inode* inode, struct file* file)
{
	--device_is_open;

	module_put(THIS_MODULE);

	printk(KERN_INFO "[hello_dev v%s] release", HELLO_VERSION);

	return 0;
}

static ssize_t device_read(struct file* filp, char __user* buffer, size_t size, loff_t* offset)
{
	ssize_t bytes_read = 0;
	if(*msg_ptr == 0) 
	{
		msg_ptr = msg;
		return 0;
	}
	while(size && *msg_ptr)
	{
		put_user(*(msg_ptr++), buffer++);
		--size;
		++bytes_read;
	}
	printk(KERN_INFO "[hello_dev v%s] read size %ld from string: %s", HELLO_VERSION, size, msg);
	return bytes_read;
} 

static ssize_t device_write(struct file* filp, const char __user* buffer, size_t size, loff_t* offset)
{
	ssize_t bytes_write = 0;
	printk(KERN_INFO "[hello_dev v%s] write %ld bytes", HELLO_VERSION, size);
	kfree(msg);
	msg = kmalloc(size+1, GFP_KERNEL);
	msg_ptr = msg;
	while(size)
	{
		get_user(*(msg_ptr++), buffer++);
		--size;
		++bytes_write;
	}
	if(*(msg_ptr-1) != 0)
		*msg_ptr = 0;
	msg_ptr = msg;
	return bytes_write;
}

module_init(hello_init);
module_exit(hello_exit);
