#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>

static struct kprobe kp;

static int pre_handler(struct kprobe *p, struct pt_regs *regs) {
    return 0;
}

void setup_anti_analysis(void) {
    int ret;
    
    kp.pre_handler = pre_handler;
    kp.symbol_name = "schedule";
    
    ret = register_kprobe(&kp);
    if (ret < 0) {
    } else {
    }
}

void cleanup_anti_analysis(void) {
    unregister_kprobe(&kp);
}