#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/crc32.h>
#include <linux/fs.h>

#define DEV_NAME "crcdev"
#define CRC_DEVICES 1
#define CRC_MAJOR 127
#define CRC_MINOR 0

typedef u32 crc_t;

static struct cdev *crc_dev;
static struct class *crc_class;

static int crcdev_open(struct inode *inode, struct file *file)
{
	crc_t *crc = kmalloc(sizeof(crc_t), GFP_KERNEL);
	if (crc == NULL) {
		printk(KERN_ERR "crc: Error memory allocation.\n");
		return -ENOMEM;
	}

	*crc = ~0;
	file->private_data = crc;

	return 0;
}

static int crcdev_release(struct inode *inode, struct file *file)
{
	if (file->private_data) {
		kfree(file->private_data);
		file->private_data = NULL;
	}

	return 0;
}

static ssize_t crcdev_read(struct file *file, char __user *buf,
			   size_t count, loff_t *offp)
{
	crc_t *crc = file->private_data;

	if (count < sizeof(crc_t))
		return -EINVAL;
	else if (count > sizeof(crc_t))
		count = sizeof(crc_t);

	if (copy_to_user(buf, crc, count)) {
		printk(KERN_ERR "crc: Error reading.\n");
		return -EFAULT;
	}

	return count;
}

static ssize_t crcdev_write(struct file *file, const char __user *buf,
			    size_t count, loff_t *offp)
{
	char *temp_buf = NULL;
	crc_t *temp_crc = file->private_data;

	if (count <= 0)
		return -EINVAL;

	temp_buf = kmalloc(count, GFP_KERNEL);
	if (!temp_buf) {
		printk(KERN_ERR "crc: Error allocating memory.\n");
		return -ENOMEM;
	}

	if (copy_from_user(temp_buf, buf, count)) {
		printk(KERN_ERR "crc: Error writing.\n");
		kfree(temp_buf);
		return -EFAULT;
	}

	*temp_crc = crc32_le(*temp_crc, temp_buf, count);
	kfree(temp_buf);

	return count;
}

static const struct file_operations crcdev_fops = {
	.owner = THIS_MODULE,
	.read = crcdev_read,
	.write = crcdev_write,
	.open = crcdev_open,
	.release = crcdev_release,
};

static char *crc_devnode(struct device *dev, umode_t *mode)
{
	if (!mode)
		return NULL;

	if (dev->devt == MKDEV(CRC_MAJOR, 0) ||
	    dev->devt == MKDEV(CRC_MAJOR, 2))
		*mode = 0666;

	return NULL;
}

static int __init crcdev_init(void)
{
	int retval;
	struct device *retdev;

	retval = register_chrdev_region(MKDEV(CRC_MAJOR, CRC_MINOR),
					CRC_DEVICES, DEV_NAME);
	if (retval < 0) {
		printk(KERN_ERR "crc: Error registering device numbers.\n");
		return retval;
	}

	crc_dev = cdev_alloc();
	if (!crc_dev ) {
		printk(KERN_ERR "crc: Error allocating device.\n");
		return -ENOMEM;
	}

	cdev_init(crc_dev, &crcdev_fops);
	retval = cdev_add(crc_dev, MKDEV(CRC_MAJOR, CRC_MINOR),
			  CRC_DEVICES);
	if (retval) {
		printk(KERN_ERR "crc: Error adding device.\n");
		return retval;
	}

	crc_class = class_create(THIS_MODULE, "crc_class");
	if (IS_ERR(crc_class)) {
		printk(KERN_ERR "crc: Error registering device class.\n");
		return PTR_ERR(crc_class);
	}

	crc_class->devnode = crc_devnode;
	retdev = device_create(crc_class, NULL,
			       MKDEV(CRC_MAJOR, CRC_MINOR), "%s", "crc");
	if (IS_ERR(retdev)) {
		printk(KERN_ERR "crc: Error creating device.\n");
		return -ENODEV;
	}

	printk(KERN_INFO "crc: Crc device init.\n");

	return 0;
}

static void __exit crcdev_exit(void)
{
	device_destroy(crc_class, MKDEV(CRC_MAJOR, CRC_MINOR));
	class_destroy(crc_class);

	if (crc_dev)
		cdev_del(crc_dev);

	unregister_chrdev_region(MKDEV(CRC_MAJOR, CRC_MINOR), CRC_DEVICES);

	printk(KERN_INFO "crc: Crc device exit.\n");
}

module_init(crcdev_init);
module_exit(crcdev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Daniil Leonov");
MODULE_DESCRIPTION("Device to get crc32 from byte stream.");