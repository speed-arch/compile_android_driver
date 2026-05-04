#include "rwProcMem_module.h"

#define MY_TASK_COMM_LEN 16

#pragma pack(push,1)
struct ioctl_request {
    char     cmd;        /* 1 字节命令 */
    uint64_t param1;     /* 参数1 */
    uint64_t param2;     /* 参数2 */
    uint64_t param3;     /* 参数3 */
    uint64_t buf_size;    /* 紧随其后的动态数据长度 */
};
struct init_device_info {
	int pid;
	int tgid;
	char my_name[MY_TASK_COMM_LEN + 1];
	char my_auxv[1024];
	int my_auxv_size;
};
struct arg_info {
	uint64_t arg_start;
	uint64_t arg_end;
};
#pragma pack(pop)

static ssize_t OnCmdInitDeviceInfo(struct ioctl_request *hdr, char __user* buf) {
	long err = 0;
	struct init_device_info* pinit_device_info = (struct init_device_info*)x_kmalloc(sizeof(struct init_device_info), GFP_KERNEL);
	if (!pinit_device_info) {
		return -ENOMEM;
	}
	printk_debug(KERN_INFO "CMD_INIT_DEVICE_INFO\n");
	memset(pinit_device_info, 0, sizeof(struct init_device_info));
	if (x_copy_from_user((void*)pinit_device_info, (void*)buf, sizeof(struct init_device_info)) == 0) {
		printk_debug(KERN_INFO "my_name:%s\n", pinit_device_info->my_name);
		printk_debug(KERN_INFO "pid:%d, tgid:%d\n", pinit_device_info->pid, pinit_device_info->tgid);
		printk_debug(KERN_INFO "my_auxv_size:%d\n", pinit_device_info->my_auxv_size);
		
		do {
			err = init_mmap_lock_offset();
			if(err) { break; }
			err = init_map_count_offset();
			if(err) { break; }
			err = init_proc_cmdline_offset(&pinit_device_info->my_auxv[0], pinit_device_info->my_auxv_size);
			if(err) { break; }
			err = init_proc_root_offset(pinit_device_info->my_name);
			if(err) { break; }
			err = init_task_next_offset();
			if(err) { break; }
			err = init_task_pid_offset(pinit_device_info->pid, pinit_device_info->tgid);
		} while(0);
	} else {
		err = -EINVAL;
	}
	kfree(pinit_device_info);
	return err;
}

static ssize_t OnCmdOpenProcess(struct ioctl_request *hdr, char __user* buf) {
	uint64_t pid = hdr->param1, handle = 0;
	struct pid * proc_pid_struct = NULL;
	printk_debug(KERN_INFO "CMD_OPEN_PROCESS\n");
	proc_pid_struct = get_proc_pid_struct(pid);
	if (!proc_pid_struct) {
		return -EINVAL;
	}
	handle = (uint64_t)proc_pid_struct;
	if (!!x_copy_to_user((void*)buf, (void*)&handle, sizeof(handle))) {
		return -EINVAL;
	}
	return 0;
}

static ssize_t OnCmdCloseProcess(struct ioctl_request *hdr, char __user* buf) {
	struct pid * proc_pid_struct = (struct pid *)hdr->param1;
	printk_debug(KERN_INFO "CMD_CLOSE_PROCESS\n");
	release_proc_pid_struct(proc_pid_struct);
	return 0;
}

static ssize_t OnCmdReadProcessMemory(struct ioctl_request *hdr, char __user* buf) {
	struct pid * proc_pid_struct = (struct pid *)hdr->param1;
	size_t proc_virt_addr = (size_t)hdr->param2;
	bool is_force_read = hdr->param3 == 1 ? true : false;
	size_t size = (size_t)hdr->buf_size;
	size_t read_size = 0;

	printk_debug(KERN_INFO "CMD_READ_PROCESS_MEMORY\n");

	if (is_force_read == false && !check_proc_map_can_read(proc_pid_struct, proc_virt_addr, size)) {
		return -EFAULT;
	}

	while (read_size < size) {
		size_t phy_addr = 0;
		size_t pfn_sz = 0;
		char *lpOutBuf = NULL;
		pte_t *pte;
		bool old_pte_can_read;
		
		phy_addr = get_proc_phy_addr(proc_pid_struct, proc_virt_addr + read_size, &pte);
		if (phy_addr == 0) { break; }

		old_pte_can_read = is_pte_can_read(pte);
		if (is_force_read) {
			if (!old_pte_can_read) {
				if (!change_pte_read_status(pte, true)) { break; }
			}
		} else if (!old_pte_can_read) {
			break;
		} 

		pfn_sz = size_inside_page(phy_addr, ((size - read_size) > PAGE_SIZE) ? PAGE_SIZE : (size - read_size));
		lpOutBuf = (char*)(buf + read_size);
		read_ram_physical_addr(false, phy_addr, lpOutBuf, pfn_sz);

		if (is_force_read && old_pte_can_read == false) {
			change_pte_read_status(pte, false);
		}
		read_size += pfn_sz;
	}
	return read_size;
}

static ssize_t OnCmdWriteProcessMemory(struct ioctl_request *hdr, char __user* buf) {
	struct pid * proc_pid_struct = (struct pid *)hdr->param1;
	size_t proc_virt_addr = (size_t)hdr->param2;
	bool is_force_write = hdr->param3 == 1 ? true : false;
	size_t size = (size_t)hdr->buf_size;
	size_t write_size = 0;

	printk_debug(KERN_INFO "CMD_WRITE_PROCESS_MEMORY\n");

	if (is_force_write == false && !check_proc_map_can_write(proc_pid_struct, proc_virt_addr, size)) {
		return -EFAULT;
	}

	while (write_size < size) {
		size_t phy_addr = 0;
		size_t pfn_sz = 0;
		char * input_buf = NULL;
		pte_t *pte;
		bool old_pte_can_write;
		
		phy_addr = get_proc_phy_addr(proc_pid_struct, proc_virt_addr + write_size, &pte);
		if (phy_addr == 0) { break; }

		old_pte_can_write = is_pte_can_write(pte);
		if (is_force_write) {
			if (!old_pte_can_write) {
				if (!change_pte_write_status(pte, true)) { break; }
			}
		} else if (!old_pte_can_write) {
			break;
		}

		pfn_sz = size_inside_page(phy_addr, ((size - write_size) > PAGE_SIZE) ? PAGE_SIZE : (size - write_size));
		input_buf = (char*)(((size_t)buf + write_size));
		write_ram_physical_addr(phy_addr, input_buf, false, pfn_sz);
		
		if (is_force_write && old_pte_can_write == false) {
			change_pte_write_status(pte, false);
		}
		write_size += pfn_sz;
	}
	return write_size;
}

static ssize_t OnCmdGetProcessMapsCount(struct ioctl_request *hdr, char __user* buf) {
	struct pid * proc_pid_struct = (struct pid *)hdr->param1;
	return get_proc_map_count(proc_pid_struct);
}

static ssize_t OnCmdGetProcessMapsList(struct ioctl_request *hdr, char __user* buf) {
	struct pid * proc_pid_struct = (struct pid *)hdr->param1;
	return get_proc_maps_list(false, proc_pid_struct, (void*)(buf), hdr->buf_size - 1);
}

static ssize_t OnCmdCheckProcessPhyAddr(struct ioctl_request *hdr, char __user* buf) {
	struct pid * proc_pid_struct = (struct pid *)hdr->param1;
	size_t proc_virt_addr = (size_t)hdr->param2;
	pte_t *pte;
	if (get_proc_phy_addr(proc_pid_struct, proc_virt_addr, &pte)) {
		return 1;
	}
	return 0;
}

static ssize_t OnCmdGetPidList(struct ioctl_request *hdr, char __user* buf) {
	return get_proc_pid_list(false, buf, hdr->buf_size);
}

static ssize_t OnCmdSetProcessRoot(struct ioctl_request *hdr, char __user* buf) {
	struct pid * proc_pid_struct = (struct pid *)hdr->param1;
	return set_process_root(proc_pid_struct);
}

static ssize_t OnCmdGetProcessRss(struct ioctl_request *hdr, char __user* buf) {
	struct pid * proc_pid_struct = (struct pid *)hdr->param1;
	uint64_t rss = read_proc_rss_size(proc_pid_struct);
	if (!!x_copy_to_user((void*)buf, &rss, sizeof(rss))) {
		return -EINVAL;
	}
	return 0;
}

static ssize_t OnCmdGetProcessCmdlineAddr(struct ioctl_request *hdr, char __user* buf) {
	struct pid * proc_pid_struct = (struct pid *)hdr->param1;
	size_t arg_start = 0, arg_end = 0;
	int res;
	struct arg_info aginfo = {0};
	res = get_proc_cmdline_addr(proc_pid_struct, &arg_start, &arg_end);
	aginfo.arg_start = (uint64_t)arg_start;
	aginfo.arg_end = (uint64_t)arg_end;
	if (!!x_copy_to_user((void*)buf, &aginfo, sizeof(aginfo))) {
		return -EINVAL;
	}
	return res;
}

static ssize_t OnCmdHideKernelModule(struct ioctl_request *hdr, char __user* buf) {
	if (g_rwProcMem_devp->is_hidden_module == false) {
		g_rwProcMem_devp->is_hidden_module = true;
		WRITE_ONCE(__this_module.state, MODULE_STATE_UNFORMED);
		list_del_rcu(&__this_module.list);
		kobject_del(&THIS_MODULE->mkobj.kobj);
		synchronize_rcu();
		memset(__this_module.name, 0, sizeof(__this_module.name));

#ifdef CONFIG_KALLSYMS
		THIS_MODULE->core_kallsyms.num_symtab = 0;
		THIS_MODULE->core_kallsyms.symtab = NULL;
		THIS_MODULE->core_kallsyms.strtab = NULL;
		THIS_MODULE->core_kallsyms.typetab = NULL;
#endif
	}
	return 0;
}

static inline ssize_t DispatchCommand(struct ioctl_request *hdr, char __user* buf) {
	switch (hdr->cmd) {
	case CMD_INIT_DEVICE_INFO: return OnCmdInitDeviceInfo(hdr, buf);
	case CMD_OPEN_PROCESS: return OnCmdOpenProcess(hdr, buf);
	case CMD_READ_PROCESS_MEMORY: return OnCmdReadProcessMemory(hdr, buf);
	case CMD_WRITE_PROCESS_MEMORY: return OnCmdWriteProcessMemory(hdr, buf);
	case CMD_CLOSE_PROCESS: return OnCmdCloseProcess(hdr, buf);
	case CMD_GET_PROCESS_MAPS_COUNT: return OnCmdGetProcessMapsCount(hdr, buf);
	case CMD_GET_PROCESS_MAPS_LIST: return OnCmdGetProcessMapsList(hdr, buf);
	case CMD_CHECK_PROCESS_ADDR_PHY: return OnCmdCheckProcessPhyAddr(hdr, buf);
	case CMD_GET_PID_LIST: return OnCmdGetPidList(hdr, buf);
	case CMD_SET_PROCESS_ROOT: return OnCmdSetProcessRoot(hdr, buf);
	case CMD_GET_PROCESS_RSS: return OnCmdGetProcessRss(hdr, buf);
	case CMD_GET_PROCESS_CMDLINE_ADDR: return OnCmdGetProcessCmdlineAddr(hdr, buf);
	case CMD_HIDE_KERNEL_MODULE: return OnCmdHideKernelModule(hdr, buf);
	default: return -EINVAL;
	}
}

static ssize_t rwProcMem_read(struct file* filp, char __user* buf, size_t size, loff_t* ppos) {
    struct ioctl_request hdr = {0};
    size_t header_size = sizeof(hdr);

    if (size < header_size) { return -EINVAL; }
    if (x_copy_from_user(&hdr, buf, header_size)) { return -EFAULT; }
    if (size < header_size + hdr.buf_size) { return -EINVAL; }

    return DispatchCommand(&hdr, buf + header_size);
}

static int rwProcMem_dev_init(void) {
	g_rwProcMem_devp = x_kmalloc(sizeof(struct rwProcMemDev), GFP_KERNEL);
	memset(g_rwProcMem_devp, 0, sizeof(struct rwProcMemDev));

#ifdef CONFIG_USE_PROC_FILE_NODE
	g_rwProcMem_devp->proc_parent = proc_mkdir(CONFIG_PROC_NODE_AUTH_KEY, NULL);
	if(g_rwProcMem_devp->proc_parent) {
		g_rwProcMem_devp->proc_entry = proc_create(CONFIG_PROC_NODE_AUTH_KEY, S_IRUGO | S_IWUGO, g_rwProcMem_devp->proc_parent, &rwProcMem_proc_ops);
		start_hide_procfs_dir(CONFIG_PROC_NODE_AUTH_KEY);
	}
#endif

	printk_debug(KERN_EMERG "loaded\n");
	return 0;
}

static void rwProcMem_dev_exit(void) {
#ifdef CONFIG_USE_PROC_FILE_NODE
	if(g_rwProcMem_devp->proc_entry) {
		proc_remove(g_rwProcMem_devp->proc_entry);
		g_rwProcMem_devp->proc_entry = NULL;
	}
	
	if(g_rwProcMem_devp->proc_parent) {
		proc_remove(g_rwProcMem_devp->proc_parent);
		g_rwProcMem_devp->proc_parent = NULL;
	}
	stop_hide_procfs_dir();
#endif
	kfree(g_rwProcMem_devp);
}

int __init init_module(void) {
    return rwProcMem_dev_init();
}

void __exit cleanup_module(void) {
    rwProcMem_dev_exit();
}

unsigned long __stack_chk_guard;

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux");
MODULE_DESCRIPTION("Linux default module");
