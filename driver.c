#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>

#include "driver.h"

#define BUF_SIZE	1024

struct driver_device {
	char *buffer;
	int value;
	struct miscdevice *mdev;
};

struct driver_device *dev;

static int driver_open(struct inode *node, struct file *file)
{
	printk(KERN_INFO "Enter: %s\n", __func__);
	file->private_data = (void *)dev;

	return 0;
}

static int driver_release(struct inode *node, struct file *file)
{
	printk(KERN_INFO "Enter: %s\n", __func__);

	return 0;
}

static ssize_t driver_read(struct file *file, char __user *buf, size_t size, loff_t *pos)
{
	int ret;
	int read_bytes;
	struct driver_device *demo = file->private_data;
	char *kbuf = demo->buffer + *pos;

	if(*pos >= BUF_SIZE)
		return 0;
	if(size > (BUF_SIZE - *pos))
		read_bytes = BUF_SIZE - *pos;
	else
		read_bytes = size;

	ret = copy_to_user(buf, kbuf, read_bytes);
	if(ret != 0)
		return -EFAULT;
	*pos += read_bytes;

	return read_bytes;
}

static ssize_t driver_write(struct file *file, const char __user *buf, size_t size, loff_t *pos)
{
	int ret;
	int write_bytes;
	struct driver_device *demo = (struct driver_device *)(file->private_data);
	char *kbuf = demo->buffer + *pos;

	if(*pos >= BUF_SIZE)
		return 0;

	if(size > (BUF_SIZE - *pos))
		write_bytes = BUF_SIZE - *pos;
	else
		write_bytes = size;

	ret = copy_from_user(kbuf, buf, write_bytes);
	if(ret != 0)
		return -EFAULT;
	*pos += write_bytes;

	return write_bytes;
}

static long driver_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct driver_device *demo = file->private_data;

	switch(cmd) {
		case DEMO_MEM_CLEAN:
			printk(KERN_INFO "cmd: clean\n");
			memset(demo->buffer, 0x00, BUF_SIZE);
			break;
		case DEMO_MEM_GETVAL:
			printk(KERN_INFO "cmd: getval\n");
			put_user(demo->value, (int *)arg);
			break;
		case DEMO_MEM_SETVAL:
			printk(KERN_INFO "cmd: setval\n");
			demo->value = (int)arg;
			break;
		default:
			break;
	}
	
	return (long)ret;
}

static struct file_operations driver_operation = {
	.open = driver_open,
	.release = driver_release,
	.read = driver_read,
	.write = driver_write,
	.unlocked_ioctl = driver_ioctl,
};

static struct miscdevice misc_struct = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "liupp",
	.fops = &driver_operation,
};

static int __init demo_init(void)
{
	int ret = -1;

	dev = (struct driver_device *)kmalloc(sizeof(struct driver_device), GFP_KERNEL);
	if(!dev) {
		printk(KERN_ERR "failed to malloc driver device\n");
		ret = -ENOMEM;
		goto ERROR_MALLOC_DEVICE;
	}

	dev->buffer = (char *)kmalloc(BUF_SIZE, GFP_KERNEL);
	if(!dev->buffer) {
		printk(KERN_ERR "malloc %d bytes failed\n", BUF_SIZE);
		ret = -ENOMEM;
		goto ERROR_MALLOC_BUFFER;
	}

	memset(dev->buffer, 0x00, BUF_SIZE);
	dev->value = 1;

	dev->mdev = &misc_struct;
	ret = misc_register(dev->mdev);
	if(ret < 0) {
		printk(KERN_ERR "failed to register misc\n");
		goto ERROR_MISC;
	}

	printk(KERN_INFO "driver init successfully\n");

	return 0;

ERROR_MISC:
	kfree(dev->buffer);
	dev->buffer = NULL;
ERROR_MALLOC_BUFFER:
	kfree(dev);
	dev = NULL;
ERROR_MALLOC_DEVICE:

	return ret;
}

static void __exit demo_exit(void)
{
	misc_deregister(dev->mdev);

	kfree(dev->buffer);
	dev->buffer = NULL;
	kfree(dev);
	dev = NULL;
	
	printk(KERN_INFO "driver exit done\n");
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_AUTHOR("Liu Panpan");
MODULE_LICENSE("GPL");
