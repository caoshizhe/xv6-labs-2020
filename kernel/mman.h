// kernel/mman.h
// kernel/mman.h
#pragma once

// mmap prot 参数
#define PROT_NONE    0x0
#define PROT_READ    0x1
#define PROT_WRITE   0x2
#define PROT_EXEC    0x4

// mmap flags 参数
#define MAP_SHARED   0x01
#define MAP_PRIVATE  0x02