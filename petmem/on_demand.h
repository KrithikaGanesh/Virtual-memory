/* On demand Paging Implementation
 * (c) Jack Lange, 2012
 */

#include <linux/module.h>


//from linux kernel
/*https://elixir.free-electrons.com/linux/v4.6/source/include/linux/vmalloc.h#L30
struct vmap_area {
	unsigned long va_start;
	unsigned long va_end;
	unsigned long flags;
	struct rb_node rb_node;         
	struct list_head list;         
	struct list_head purge_list;   
	struct vm_struct *vm;
	struct rcu_head rcu_head;
};
*/
struct vaddr_reg {
    /* You can use this to record each virtual address allocation */
	u64 va_start;
	u64 va_end;
	u64 num_pages;
	u8 alloc_status;
	struct list_head vm_list;
	
};

struct mem_map {
    /* Add your own state here */
	struct list_head track_memalloc;
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
