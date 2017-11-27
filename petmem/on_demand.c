/* On demand Paging Implementation
 * (c) Krithika, 2017
 */

#include <linux/slab.h>


#include "petmem.h"
#include "on_demand.h"
#include "pgtables.h"





// https://www.ibm.com/developerworks/library/l-kernel-memory-access/index.html
struct mem_map *
petmem_init_process(void)
{
	printk("PETMEM INIT PROCESS");

	//Virtual memory 

	struct vaddr_reg * giant_Node = (struct vaddr_reg *) kmalloc(sizeof(struct vaddr_reg), GFP_KERNEL);
	giant_Node->va_start = PETMEM_REGION_START;
	giant_Node->va_end = PETMEM_REGION_END;
	giant_Node->num_pages = ((PETMEM_REGION_END - PETMEM_REGION_START) >> 12);
	giant_Node->alloc_status = FREE;
	// We need a LL of available memory, all nodes are FREE
	INIT_LIST_HEAD(&(giant_Node->vm_list));
	printk("start: %llu",giant_Node->va_start);
	printk("end: %llu",giant_Node->va_end);
	printk("numpages: %llu",giant_Node->num_pages);
	printk("status: %d",giant_Node->alloc_status); //FREE =11

	//  mmap tracks free spaces

	struct mem_map * mmap_init = (struct mem_map *)kmalloc(sizeof(struct mem_map), GFP_KERNEL);
	INIT_LIST_HEAD(&(mmap_init->track_memalloc));
	
	
	// add all free nodes to mmap LL
	list_add(&(giant_Node->vm_list),&(mmap_init->track_memalloc));

	return mmap_init;

	/* LOG
	[ 8938.241542] start: 68719476736
	[ 8938.241543] end: 206158430208
	[ 8938.241543] numpages: 33554432
	[ 8938.241544] status: 11
	*/



   
}


void
petmem_deinit_process(struct mem_map * map)
{
    
}


uintptr_t
petmem_alloc_vspace(struct mem_map * map,
		    u64              num_pages)
{
    printk("Memory allocation\n");

    return 0;
}

void
petmem_dump_vspace(struct mem_map * map)
{
    return;
}




// Only the PML needs to stay, everything else can be freed
void
petmem_free_vspace(struct mem_map * map,
		   uintptr_t        vaddr)
{
    printk("Free Memory\n");
    return;
}


/* 
   error_code:
       1 == not present
       2 == permissions error
*/

int
petmem_handle_pagefault(struct mem_map * map,
			uintptr_t        fault_addr,
			u32              error_code)
{
    printk("Page fault!\n");

    return -1;
}
