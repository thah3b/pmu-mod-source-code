#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>

#define DEVICE_NAME "pmu-mod-example"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple character kernel module");
MODULE_VERSION("0.1");

static int major_number;
static char message[256] = "Hello from the kernel!\n";
static int message_size = sizeof(message);

static struct cdev my_cdev;
static struct class *my_class;

static int device_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int device_release(struct inode *inode, struct file *file)
{
    return 0;
}

static ssize_t device_read(struct file *file, char __user *buffer, size_t length, loff_t *offset)
{
    if (*offset >= message_size)
        return 0;

    if (*offset + length > message_size)
        length = message_size - *offset;

    if (copy_to_user(buffer, message + *offset, length) != 0)
        return -EFAULT;

    *offset += length;
    return length;
}

static ssize_t device_write(struct file *file, const char __user *buffer, size_t length, loff_t *offset)
{
    if (copy_from_user(message, buffer, length) != 0)
        return -EFAULT;

    message_size = length;
    return length;
}

static struct file_operations fops = {
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .release = device_release,
};

static int __init char_dev_init(void)
{
   // Allocate and initialize a range of character devices
    if (alloc_chrdev_region(&major_number, 0, 1, DEVICE_NAME) < 0)
    {
        printk(KERN_ALERT "Failed to allocate character device region\n");
        return -1;
    }

    // Initialize the character device
    cdev_init(&my_cdev, &fops);
    my_cdev.owner = THIS_MODULE;

    // Add the character device to the system
    if (cdev_add(&my_cdev, major_number, 1) < 0)
    {
        printk(KERN_ALERT "Failed to add character device\n");
        unregister_chrdev_region(major_number, 1);
        return -1;
    }

    // Create a class for the device
    my_class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(my_class))
    {
        printk(KERN_ALERT "Failed to create class\n");
        cdev_del(&my_cdev);
        unregister_chrdev_region(major_number, 1);
        return PTR_ERR(my_class);
    }

    // Create the device node
    if (device_create(my_class, NULL, major_number, NULL, DEVICE_NAME) == NULL)
    {
        printk(KERN_ALERT "Failed to create device node\n");
        class_destroy(my_class);
        cdev_del(&my_cdev);
        unregister_chrdev_region(major_number, 1);
        return -1;
    }

    printk(KERN_INFO "Registered correctly with major number %d\n", major_number);
    return 0;
}

static void __exit char_dev_exit(void)
{
    // Remove the device node
    device_destroy(my_class, major_number);

    // Destroy the class
    class_destroy(my_class);

    // Remove the character device
    cdev_del(&my_cdev);

    // Unregister the character device region
    unregister_chrdev_region(major_number, 1);

    printk(KERN_INFO "Unregistered the character device\n");
}

module_init(char_dev_init);
module_exit(char_dev_exit);
