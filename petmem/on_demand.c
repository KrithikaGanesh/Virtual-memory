/* On demand Paging Implementation
 * (c) Krithika, 2017
 */

#include <linux/slab.h>


#include "petmem.h"
#include "on_demand.h"
#include "pgtables.h"

#define PHYSICAL_OFFSET(x) (((u64)x) & 0xfff)
#define PAGE_IN_USE 1
#define PAGE_NOT_IN_USE 2
#define ERROR_PERMISSION 2

void attempt_free_physical_address(uintptr_t address);

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
    struct list_head * node, * temp;
	struct vaddr_reg *entry;
	list_for_each_safe(node, temp, &(map->track_memalloc)){
		entry = list_entry(node, struct vaddr_reg, vm_list);
        attempt_free_physical_address(entry->va_start);
        invlpg(entry->va_start);
		list_del(node);
		kfree(entry);
	}
	kfree(map);
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

int is_entire_page_free(void * page_structure){
    int i = 0;
    for(i = 0; i < 512; i++){
        int offset = i * 8;
        pte64_t * page = __va(page_structure + offset);
        if(page->present == 1){
            printk("Page index is: %d\n", i);
            return PAGE_IN_USE;
        }
    }
    return PAGE_NOT_IN_USE;
}


void attempt_free_physical_address(uintptr_t address){

 	pml4e64_t * cr3;
	pdpe64_t * pdp, *pdp_table;
	pde64_t * pde, *pde_table;
	pte64_t * pte, *pte_table;
    void * actual_mem;
    

    cr3 = (pml4e64_t *) (CR3_TO_PML4E64_VA( get_cr3() ) + PML4E64_INDEX( address ) * 8);
    if(!cr3->present) {
        return;
    }
    pdp = (pdpe64_t *)__va( BASE_TO_PAGE_ADDR( cr3->pdp_base_addr ) + (PDPE64_INDEX( address ) * 8)) ;
    pdp_table = (pdpe64_t *)( BASE_TO_PAGE_ADDR( cr3->pdp_base_addr )) ;
    if(!pdp->present) {
        return;
    }

    pde = (pde64_t *)__va(BASE_TO_PAGE_ADDR( pdp->pd_base_addr ) + PDE64_INDEX( address )* 8);
    pde_table = (pde64_t *)(BASE_TO_PAGE_ADDR( pdp->pd_base_addr ));
    if(!pde->present) {
        return;
    }

    pte = (pte64_t *)__va( BASE_TO_PAGE_ADDR( pde->pt_base_addr ) + PTE64_INDEX( address ) * 8 );
    pte_table = (pte64_t *)( BASE_TO_PAGE_ADDR( pde->pt_base_addr ));
    if(!pte->present) {
        return;
    }

    actual_mem = (void *)( BASE_TO_PAGE_ADDR( pte->page_base_addr ) + PHYSICAL_OFFSET( address ) );
    petmem_free_pages((uintptr_t)actual_mem, 1);
   
	  
	pte->writable = 0;
    pte->user_page = 0;
    pte->present = 0;
    pte->page_base_addr = 0;
    invlpg(pte_table);
    if(is_entire_page_free((void *)pte_table) == PAGE_NOT_IN_USE){
        printk("Freeing pte table\n");
       petmem_free_pages((uintptr_t)pte_table, 1);
    }
    else{
        return;
    }
    pde->writable = 0;
    pde->user_page = 0;
    pde->present = 0;
    pde->pt_base_addr = 0;
    if(is_entire_page_free((void *)pde_table) == PAGE_NOT_IN_USE){
        printk("Freeing pde table\n");
       petmem_free_pages((uintptr_t)pde_table, 1);
    }
    else{
        return;
    }
    pdp->writable = 0;
    pdp->user_page = 0;
    pdp->present = 0;
    pdp->pd_base_addr = 0;
    if(is_entire_page_free((void *)pdp_table) == PAGE_NOT_IN_USE){
        printk("Freeing pdp table\n");
       petmem_free_pages((uintptr_t)pdp_table, 1);
    }
    else{
        return;
    }
    cr3->present = 0;
    cr3->pdp_base_addr = 0;
    cr3->user_page = 0;
    cr3->writable = 0;

}




void
petmem_free_vspace(struct mem_map * map,
		   uintptr_t        vaddr)
{
    	printk("Free Memory\n");
	struct vaddr_reg *node, *foundnode, *next_node, *prev_node;
	foundnode=NULL;
	int i;
	pml4e64_t * pml;
	pdpe64_t * pdp;
	pde64_t * pde;
	pte64_t * pte;

	list_for_each_entry(node,&(map->track_memalloc),vm_list){
		
			if(node->va_start==vaddr){
				foundnode=node;
				break;
			}
		
	}
	
	foundnode->alloc_status = FREE;
	attempt_free_physical_address(foundnode->va_start);

	for(i = 0; i< foundnode->num_pages;i++)
	{
		invlpg(foundnode->va_start + (i*4096));

	}
	
	next_node = list_entry(foundnode->vm_list.next, struct vaddr_reg, vm_list);
	prev_node = list_entry(foundnode->vm_list.prev, struct vaddr_reg, vm_list);
	
	if(next_node->alloc_status == FREE)
	{
		list_del(foundnode->vm_list.next);
		foundnode->num_pages= foundnode->num_pages+ next_node->num_pages;
		kfree(next_node);
	}
	if(prev_node->alloc_status == FREE)
	{
		foundnode->va_start= prev_node->va_start;		
		list_del(foundnode->vm_list.prev);
		foundnode->num_pages= foundnode->num_pages+ prev_node->num_pages;
		kfree(prev_node);
	}
	

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


int petmem_handle_pagefault(struct mem_map * map, uintptr_t fault_addr, u32 error_code) {
	pml4e64_t * pml;
	pdpe64_t * pdp;
	pde64_t * pde;
	pte64_t * pte;

    pml = (pml4e64_t *) (CR3_TO_PML4E64_VA( get_cr3() ) + PML4E64_INDEX( fault_addr ) * 8);
	
    if(!pml->present) {
	printk("PML");
        pte64_t * pml_page = (uintptr_t)__va(petmem_alloc_pages(1));
	memset((void *)pml_page, 0, 4096);	    
	pml->present = 1;
	pml->pdp_base_addr = PAGE_TO_BASE_ADDR( __pa(pml_page));
    }

    pdp = (pdpe64_t *)__va( BASE_TO_PAGE_ADDR(pml->pdp_base_addr ) + (PDPE64_INDEX( fault_addr ) * 8)) ;
    if(!pdp->present) {
	printk("PDP");        
	pdpe64_t * pdp_page = (pdpe64_t *)__va(petmem_alloc_pages(1));
    	memset((void *)pdp_page, 0, 4096);	    
	pdp->present = 1;
	pdp->pd_base_addr = PAGE_TO_BASE_ADDR( __pa(pdp_page));
        pdp->writable = 1;
        pdp->user_page = 1;

    }

    pde = (pde64_t *)__va(BASE_TO_PAGE_ADDR( pdp->pd_base_addr ) + PDE64_INDEX( fault_addr )* 8);
    if(!pde->present) {
	printk("PDE");        
	pde64_t * pde_page = (pde64_t *)__va(petmem_alloc_pages(1));	
   	memset((void *)pde_page, 0, 4096);	    
	pde->present = 1;
	pde->pt_base_addr = PAGE_TO_BASE_ADDR( __pa(pde_page));
        pde->writable = 1;
        pde->user_page = 1;
    }

    pte = (pte64_t *)__va( BASE_TO_PAGE_ADDR( pde->pt_base_addr ) + PTE64_INDEX( fault_addr ) * 8 );

    if(!pte->present) {
	printk("PTE");        
	pte64_t * datapage = (pte64_t *)__va(petmem_alloc_pages(1));    	
       	memset((void *)datapage, 0, 4096);    
	pte->present = 1;
	pte->page_base_addr = PAGE_TO_BASE_ADDR( __pa(datapage));        
        pte->writable = 1;
        pte->user_page =1;
    }
	return 0;
	/*
	Allocated 1 page at 0x1000000000
	SIGSEGV
	Hello World!
	*/

}


