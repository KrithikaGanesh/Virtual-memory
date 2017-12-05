/* On demand Paging Implementation
 * (c) Jack Lange, 2012
 */

#include <linux/module.h>


struct vaddr_reg {
    int occupied;
    uintptr_t addr_start;
//    uintptr_t addr_end;
    u64 size;
    struct vaddr_reg * next;
    struct vaddr_reg * prev;    
};

struct mem_map {
    uintptr_t br;
    struct vaddr_reg * start;
    struct vaddr_reg * end;
    u64 cr3;
};



struct mem_map * petmem_init_process(void);

void petmem_deinit_process(struct mem_map * map);

uintptr_t petmem_alloc_vspace(struct mem_map * map,
			      u64              num_pages);

void petmem_free_vspace(struct mem_map * map,
			uintptr_t        vaddr);

void petmem_dump_vspace(struct mem_map * map);

// How do we get error codes??
int petmem_handle_pagefault(struct mem_map * map,
			    uintptr_t        fault_addr,
			    u32              error_code);

void free_pagetable(struct vaddr_reg * node,uintptr_t vaddr, u64 cr3);
