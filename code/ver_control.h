#ifndef VERSION_CONTROL_H_
#define VERSION_CONTROL_H_

// 独立内核模块入口模式
#define CONFIG_MODULE_GUIDE_ENTRY

// 生成proc用户层交互节点文件
#define CONFIG_USE_PROC_FILE_NODE
// 隐蔽通信密钥
#define CONFIG_PROC_NODE_AUTH_KEY "e84523d7b60d5d341a7c4d1861773ecd"

// 打印内核调试信息
#define CONFIG_DEBUG_PRINTK

#ifndef KERNEL_VERSION
#define KERNEL_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))
#endif
#ifndef MY_LINUX_VERSION_CODE 
#define MY_LINUX_VERSION_CODE KERNEL_VERSION(5,10,43)
#endif

#ifdef CONFIG_DEBUG_PRINTK
#define printk_debug printk
#else
static inline void printk_debug(char *fmt, ...) {}
#endif

#endif /* VERSION_CONTROL_H_ */
