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
	printk("PM_INIT_PROC:start: %llu",giant_Node->va_start);
	printk("PM_INIT_PROC:end: %llu",giant_Node->va_end);
	printk("PM_INIT_PROC:numpages: %llu",giant_Node->num_pages);
	printk("PM_INIT_PROC:status: %d",giant_Node->alloc_status); //FREE =11

	//  mmap tracks all allocations, FREE and ALLOCATED

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

//https://elixir.free-electrons.com/linux/v4.6/source/mm/vmalloc.c#L1736
uintptr_t
petmem_alloc_vspace(struct mem_map * map,
		    u64              num_pages)
{
   	
	printk("Memory allocation\n");
	//Iterate through vmlist to find free node, less than num_pages asked
	
	struct vaddr_reg *node, *foundnode, *free_node;
	foundnode=NULL;
	list_for_each_entry(node,&(map->track_memalloc),vm_list){
		
			if(node->num_pages>=num_pages && node->alloc_status == FREE){
				foundnode=node;
				break;
			}
		
	}
	
	foundnode->alloc_status = ALLOCATE;
	
	
	//Handle size of node 
	/* if foundnode num_apges same as requested same, return start_addr of node
	   else 
		create a new node of left over pages and add to Virtual address space
	*/
	if(foundnode->num_pages==num_pages)
	{
		return foundnode->num_pages;
	}
	
	u64 foundnode_num_pages= foundnode->num_pages;
	u64 left_over_pages = foundnode_num_pages - num_pages; //left over pages
	printk("PM:ALLOC: %llu",left_over_pages);
	
	free_node = (struct vaddr_reg*)kmalloc(sizeof(struct vaddr_reg), GFP_KERNEL);
	
	free_node->va_start = foundnode->va_start + (num_pages<<12);	
	free_node->alloc_status = FREE;
	free_node->num_pages = left_over_pages;
	printk("PM:start: %llu",foundnode->va_start);

	foundnode->num_pages = num_pages; //requested
	INIT_LIST_HEAD(&(free_node->vm_list));
	list_add(&(free_node->vm_list), &(foundnode->vm_list));
	return foundnode->va_start;
	/*LOG
	petmem ioctl
	[  561.570498] Requested allocation of 8192 bytes
	[  561.570500] Memory allocation
	[  561.570501] PM:ALLOC: 33554430
	[  561.570504] PM:start: 68719476736
	*/
	
	
	
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
/*
void allocate_pg_for_table(void *mem)
{
	unintptr_t pg_table_pg = (uintptr_t)__va(petmem_alloc_pages(1));
	(pte64_t *) mem->present=1;
	(pte64_t *) mem->va_start= PAGE_TO_BASE_ADDR( __pa(pg_table_pg ));
	
}*/
int
petmem_handle_pagefault(struct mem_map * map,
			uintptr_t        fault_addr,
			u32              error_code)
{
	pml4e64_t * pmlbase;
	pdpe64_t * pdpbase;
	pde64_t * pdebase;
	pte64_t * ptebase;
	int pmlindex, pdpindex, pdeindex, pteindex;
  
	//cr3 points to the base of the PML4E64 director
	pmlbase = (pml4e64_t *) (CR3_TO_PML4E64_VA( get_cr3() )); 
	pmlindex = PML4E64_INDEX( fault_addr );

	
	if(!pmlbase[pmlindex].present){//need to allocate page for }
	printk("PG_FAULT: PMLbase %d", pmlbase[pmlindex].present);
	
	pdpbase = (pdpe64_t *)__va(BASE_TO_PAGE_ADDR(pmlbase[pmlindex].pdp_base_addr ));
	pdpindex = PML4E64_INDEX( fault_addr );

	if(!pdpbase[pdpindex].present){}
	printk("PG_FAULT: PDPbase %d", pdpbase[pdpindex].present);
	
	pdebase= (pde64_t *)__va(BASE_TO_PAGE_ADDR( pdpbase[pdpindex].pd_base_addr));
	pdeindex= PDPE64_INDEX( fault_addr );

	if(!pdebase[pdeindex].present){}
	printk("PG_FAULT: PDEbase %d", pdebase[pdeindex].present);
	
	ptebase = (pte64_t *)__va( BASE_TO_PAGE_ADDR( pdebase[pdeindex].pt_base_addr ));
	pteindex = PTE64_INDEX( fault_addr );
	
	if(!ptebase[pteindex].present){} 
	printk("PG_FAULT: PTEbase %d", ptebase[pteindex].present);

	return -1;
	/*
	page fault attributes
	[15453.917626] PG_FAULT: PMLbase 1
	[15453.917627] PG_FAULT: PDPbase 1
	[15453.917628] PG_FAULT: PDEbase 0
	[15453.917629] PG_FAULT: PTEbase 1
	*/
}

