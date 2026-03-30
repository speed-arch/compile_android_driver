#include <linux/module.h>
#include <linux/tty.h>
#include <linux/miscdevice.h>
#include <linux/random.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include "comm.h"
#include "memory.h"
#include "process.h"
#include "stealth.h"

void setup_anti_analysis(void);
void cleanup_anti_analysis(void);
void setup_stealth_timer(void);
void cleanup_stealth_timer(void);

unsigned long __stack_chk_guard;
static char device_name[64];
static bool stealth_enabled = false;

static struct miscdevice misc_dev;

static void select_driver_name(void) {
    unsigned int random;
    
    random = jiffies;
    switch (random % 8) {
        case 0:
            strncpy(device_name, "0:0:0:1000", sizeof(device_name)-1);
            break;
        case 1:
            strncpy(device_name, "0:0:0:2000", sizeof(device_name)-1);
            break;
        case 2:
            strncpy(device_name, "0:0:0:3000", sizeof(device_name)-1);
            break;
        case 3:
            strncpy(device_name, "0:0:0:4000", sizeof(device_name)-1);
            break;
        case 4:
            strncpy(device_name, "0:0:0:5000", sizeof(device_name)-1);
            break;
        case 5:
            strncpy(device_name, "0:0:0:6000", sizeof(device_name)-1);
            break;
        case 6:
            strncpy(device_name, "0:0:0:7000", sizeof(device_name)-1);
            break;
        case 7:
            strncpy(device_name, "0:0:0:8000", sizeof(device_name)-1);
            break;
    }
    
    device_name[sizeof(device_name)-1] = '\0';
}

int dispatch_open(struct inode *node, struct file *file)
{
    return 0;
}

int dispatch_close(struct inode *node, struct file *file)
{
    return 0;
}


long dispatch_ioctl(struct file *const file, unsigned int const cmd, unsigned long const arg)
{
    static COPY_MEMORY cm;
    static MODULE_BASE mb;
    static char key[0x100] = {0};
    static char name[0x100] = {0};
    static bool is_verified = false;

    if (cmd == OP_INIT_KEY && !is_verified)
    {
        if (copy_from_user(key, (void __user *)arg, sizeof(key) - 1) != 0)
        {
            return -1;
        }
    }
    
    switch (cmd)
    {
    case OP_READ_MEM:
    {
        if (copy_from_user(&cm, (void __user *)arg, sizeof(cm)) != 0)
        {
            return -1;
        }
        if (read_process_memory(cm.pid, cm.addr, cm.buffer, cm.size) == false)
        {
            return -1;
        }
        break;
    }
    case OP_WRITE_MEM:
    {
        if (copy_from_user(&cm, (void __user *)arg, sizeof(cm)) != 0)
        {
            return -1;
        }
        if (write_process_memory(cm.pid, cm.addr, cm.buffer, cm.size) == false)
        {
            return -1;
        }
        break;
    }
    case OP_MODULE_BASE:
    {
        if (copy_from_user(&mb, (void __user *)arg, sizeof(mb)) != 0 || 
            copy_from_user(name, (void __user *)mb.name, sizeof(name) - 1) != 0)
        {
            return -1;
        }
        mb.base = get_module_base(mb.pid, name);
        if (copy_to_user((void __user *)arg, &mb, sizeof(mb)) != 0)
        {
            return -1;
        }
        break;
    }
    case OP_DISABLE_STEALTH:
        if (stealth_enabled) {
            disable_stealth_mode();
            stealth_enabled = false;
        }
        break;
    default:
        break;
    }
    return 0;
}

struct file_operations dispatch_functions = {
    .owner = THIS_MODULE,
    .open = dispatch_open,
    .release = dispatch_close,
    .unlocked_ioctl = dispatch_ioctl,
};

int __init kuangshi_entry(void)
{
    int ret;
        
    select_driver_name();
      
    misc_dev.minor = MISC_DYNAMIC_MINOR;
    misc_dev.name = device_name;
    misc_dev.fops = &dispatch_functions;
    misc_dev.mode = 0666;

    hide_proc_entry();
    
    ret = misc_register(&misc_dev);
    if (ret) {
        return ret;
    }

    setup_stealth_timer();
    setup_anti_analysis();

    return 0;
}

void __exit kuangshi_unload(void)
{
    
    if (stealth_enabled) {
        disable_stealth_mode();
    }
    
    cleanup_stealth_timer();
    cleanup_anti_analysis();    
    misc_deregister(&misc_dev);
}

module_init(kuangshi_entry);
module_exit(kuangshi_unload);

MODULE_DESCRIPTION("");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("");