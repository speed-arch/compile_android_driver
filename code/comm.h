#ifndef _COMM_H
#define _COMM_H

#include <linux/types.h>

typedef struct _COPY_MEMORY
{
    pid_t pid;
    uintptr_t addr;
    void *buffer;
    size_t size;
} COPY_MEMORY, *PCOPY_MEMORY;

typedef struct _MODULE_BASE
{
    pid_t pid;
    char *name;
    uintptr_t base;
} MODULE_BASE, *PMODULE_BASE;

typedef struct _HARDWARE_BREAKPOINT
{
    pid_t pid;
    uintptr_t addr;
    int type;
    int size;
    int index;
} HARDWARE_BREAKPOINT, *PHARDWARE_BREAKPOINT;

enum OPERATIONS
{
    OP_INIT_KEY = 0x10101,
    OP_READ_MEM = 0x20202,
    OP_WRITE_MEM = 0x30303,
    OP_MODULE_BASE = 0x40404,
    OP_ENABLE_STEALTH = 0x60606,
    OP_DISABLE_STEALTH = 0x70707,
};

#endif