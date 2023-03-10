
/*I Took some of this code from Tutorial 3 on the Moodle*/
// Declare what kind of code we want
// from the header files. Defining __KERNEL__
// and MODULE allows us to access kernel-level
// code not usually available to userspace programs.
#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE
#define SUCCESS 0

#include <linux/kernel.h>  /* We're doing kernel work */
#include <linux/module.h>  /* Specifically, a module */
#include <linux/fs.h>      /* for register_chrdev */
#include <linux/uaccess.h> /* for get_user and put_user */
#include <linux/string.h>  /* for memset. NOTE - not string.h!*/
#include <linux/slab.h>    /*For The kmalloc() flag*/
#include <linux/init.h>
#include "message_slot.h"

/*The data structure that I chose for the message slot ia an array of  linked list of channels.
 Every device has it's list of used channels in the index of it's minor in the array.
every channel will contain the field of the struct channel which are the channel id,
the length of the last message that was written to the channel, the last message data
(a pointer to an array in order to maintain the complexity O(C*M)) and a pointer to the next channel on the list.
In the field of private data I will save a pointer to the last channel.
 */

LIST_OF_CHANNELS *devices[256];
MODULE_LICENSE("GPL");

//================== DEVICE FUNCTIONS ===========================
// device_open
static int device_open(struct inode *inode,
                       struct file *file)
{
    int minor = iminor(inode);
    printk("Invoking device_open(%p)minor number: %d\n", file, minor);

    if (minor > 256)
    {
        printk("This minor number is not allowed(%d)\n", minor);
        return -ENOMEM;
    }
    if (devices[minor] == NULL)
    {
        devices[minor] = kmalloc(sizeof(LIST_OF_CHANNELS), GFP_KERNEL);
        if (devices[minor] == NULL)// Checks the kmalloc
        {
            return -EAGAIN;
        }
    }
    return SUCCESS;
}

// device_read
static ssize_t device_read(struct file *file,
                           char __user *buffer,
                           size_t length,
                           loff_t *offset)
{
    int last_msg_len;
    CHANNEL_LINK channel;
    if (file->private_data == NULL)
    {
        return -EINVAL;
    }
    channel = (CHANNEL_LINK)(file->private_data);

    printk("Invoking device_read(%p,%ld)\n", file, length);
    if (channel == NULL)
    {

        return -EINVAL;
    }
    last_msg_len = channel->last_msg_len;

    if (channel->channel_id == 0 || buffer == NULL)
    {
        return -EINVAL;
    }
    if (last_msg_len == 0) // No message exists on the channel
    {

        return -EWOULDBLOCK;
    }
    if (length < last_msg_len) // The provided buffer length is too small to hold the last message written on the channel
    {
        return -ENOSPC;
    }

    if (__copy_to_user(buffer, channel->last_message_data, last_msg_len) != 0) // We want an atomic read
    {

        return -EAGAIN;
    }

    return last_msg_len;
}
//---------------------------------------------------------------
// a processs which has already opened
// the device file attempts to write to it
static ssize_t device_write(struct file *file,
                            const char __user *buffer,
                            size_t length,
                            loff_t *offset)
{
    CHANNEL_LINK channel;

    char *the_message = (char *)kmalloc(length, GFP_KERNEL); // We want the GFP_KERNEL flag

    if (the_message == NULL) // Checks the kmalloc
    {
        return -EAGAIN;
    }
    if (file->private_data == NULL)
    {
        return -EINVAL;
    }

    channel = (CHANNEL_LINK)(file->private_data);

    if (channel == NULL)
    {
        return -EINVAL;
    }
    else if (channel->last_message_data != NULL)
    {

        kfree(channel->last_message_data);
    }

    printk("Invoking device_write(%p,%ld)\n", file, length);
    if (channel->channel_id == 0 || buffer == NULL) // No channel has been set on this fd
    {
        return -EINVAL;
    }
    if (length == 0 || length > MAX_MESSAGE_SIZE)
    {
        return -EMSGSIZE;
    }
    if (__copy_from_user(the_message, buffer, length) != 0)
    {
        return -EAGAIN;
    }

    channel->last_msg_len = length; //Update the last message data
    channel->last_message_data = the_message;

    return length;
}
/*This function checks if a given channel id is already in the list. If it is then update the last channel on the private data, and
else the function creates a new channel with the given channel id and adds it to the list of the correct minor.
 */
int find_exist_channel(struct file *file, LIST_OF_CHANNELS *device, unsigned long arg)
{
    int curr_channels;
    CHANNEL *temp = (CHANNEL *)device->head;

    while (temp->next != NULL)
    {

        if (temp->channel_id == arg) // This channel is already exist
        {

            file->private_data = (void *)temp;

            return SUCCESS;
        }
        temp = temp->next;
    }

    if (temp->channel_id == arg) // Last channel
    {
        printk("After While\n %ld", temp->channel_id);

        file->private_data = (void *)temp;

        return SUCCESS;
    }

    temp->next = kmalloc(sizeof(CHANNEL), GFP_KERNEL); // Creates new channel with the given channel id
    if (temp->next == NULL)// Checks the kmalloc
    {
        return -EAGAIN;
    }
    curr_channels = devices[iminor(file->f_inode)]->curr_num_of_channels;
    devices[iminor(file->f_inode)]->curr_num_of_channels = curr_channels + 1;

    if (curr_channels + 1 >= MAX_OF_CHANNEL)
    {
        return -ENOMEM;
    }
    temp->next->channel_id = arg; //Update the channel
    temp->next->last_message_data = NULL;
    temp->next->last_msg_len = 0;
    temp->next->next = NULL;

    file->private_data = (void *)(temp->next);
    return SUCCESS;
}

/*This function creates and adds a new channel with a given channel id to an empty list of channels.*/
int create_new_channel(struct file *file, LIST_OF_CHANNELS *device, unsigned long arg)
{

    // create new channel
    CHANNEL_LINK new_channel = kmalloc(sizeof(struct channel), GFP_KERNEL); // The first channel
    if (new_channel == NULL)                                                // Checks the kmalloc
    {
        return -EAGAIN;
    }
    //Update the channel
    new_channel->channel_id = arg;
    new_channel->last_msg_len = 0;
    new_channel->last_message_data = NULL;
    device->head = new_channel;
    device->head->next = NULL;
    device->curr_num_of_channels++;
    file->private_data = (void *)(new_channel);

    return SUCCESS;
}

// device_ioctl
static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    LIST_OF_CHANNELS *device;
    printk("Invoking device_ioctl with channel id: %ld\n", arg);

    device = devices[iminor(file->f_inode)];

    if (cmd != MSG_SLOT_CHANNEL || arg == 0)
    {
        return -EINVAL;
    }

    if (device->head != NULL) // This device sets channels
    {
        printk("head_of_devuce!=NULL");
        return find_exist_channel(file, device, arg);
    }
    else // head_of_device = NULL ,We need to create a new channel
    {

        return create_new_channel(file, device, arg);
    }
}

//= ========== == == ==== = DEVICE SETUP == == == == == == == == == == == == == == =

// This structure will hold the functions to be called
// when a process does something to the device we created

struct file_operations Fops = {
    .owner = THIS_MODULE, // Required for correct count of module usage. This prevents the module from being removed while used.
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .unlocked_ioctl = device_ioctl,

};

//---------------------------------------------------------------
// Initialize the module - Register the character device
static int __init module_init_cd(void)
{

    // Negative values signify an error
    if (register_chrdev(MAJOR_NUMBER, MESSAGE_SLOT, &Fops) < 0)
    {
        printk(KERN_ALERT "%s registraion failed for  %d\n",
               MESSAGE_SLOT, MAJOR_NUMBER);

        return -EFAULT;
    }
    printk("Registeration is successful. "
           "The major device number is %d.\n",
           MAJOR_NUMBER);
    printk("If you want to talk to the device driver,\n");
    printk("you have to create a device file:\n");

    return SUCCESS;
}

//---------------------------------------------------------------
static void __exit simple_cleanup(void)
{
    // Unregister the device
    // Should always succeed
    int i;
    printk("Invoking exit: \n");

    for (i = 0; i < 256; i++)
    {
        CHANNEL_LINK temp;

        if (devices[i] != NULL)
        {

            CHANNEL_LINK head = devices[i]->head;

            while (head != NULL)
            {

                temp = head;
                head = head->next;

                if (temp->last_message_data != NULL)
                {

                    kfree(temp->last_message_data);
                }
                kfree(temp);
            }

            kfree(devices[i]);
        }
    }

    unregister_chrdev(MAJOR_NUMBER, MESSAGE_SLOT);
}

//---------------------------------------------------------------
module_init(module_init_cd);
module_exit(simple_cleanup);

//========================= END OF FILE =========================
