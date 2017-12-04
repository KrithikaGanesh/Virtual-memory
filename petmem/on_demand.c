/* On demand Paging Implementation
 * (c) Jack Lange, 2012
 */

#include <linux/slab.h>


#include "petmem.h"
#include "on_demand.h"
#include "pgtables.h"






struct mem_map *
petmem_init_process(void)
{
    printk("INIT_PROCESS");
    struct mem_map * map = kmalloc(sizeof(struct mem_map), GFP_KERNEL);
    struct vaddr_reg * reg = kmalloc(sizeof(struct vaddr_reg), GFP_KERNEL);
    reg->occupied = 0;
    map->br = reg->addr_start = PETMEM_REGION_START;
//    reg->addr_end = PETMEM_REGION_END;
    reg->next = reg->prev = NULL;
    reg->size = NULL;
    map->start = map->end = reg;
    printk("START = %lx",PETMEM_REGION_START);
    printk("map start = %lx",map->br);
    return map;
}


void
petmem_deinit_process(struct mem_map * map)
{
  struct vaddr_reg * temp, * node;
  for(node = map->start; node != NULL; node = temp) {
    free_pagetable(node,node->addr_start);
    temp = node->next;
    kfree(node);
  }
  kfree(map);
}


uintptr_t
petmem_alloc_vspace(struct mem_map * map,
		    u64              num_pages)
{
    printk("Memory Allocation Start");
    uintptr_t * returnaddr;
    struct vaddr_reg * reg, * next_vaddr;
    if(map == NULL) {
      printk("Map is NULL ??");
      return NULL;
    }
    do{
      printk("Inside Do");
      reg = map->start;
      if(reg == NULL) {
        printk("reg == NULL???");
        return NULL;
      }
      /*if(reg->occupied){
        printk("Already OCCUPIED ?");
        return NULL;
      }*/
      if(!reg->occupied){
         printk("FREE NODE Found");
         if(reg->size == NULL || reg->size > num_pages) {
           printk("SIZE IS NULL || > THAN NUM_PAGES");
           next_vaddr = kmalloc(sizeof(struct vaddr_reg), GFP_KERNEL);
           next_vaddr->addr_start = reg->addr_start + num_pages * PAGE_SIZE_4KB;
           printk("Next Block Start_addr = %lx", next_vaddr->addr_start);
           reg->next = next_vaddr;
           next_vaddr->prev = reg;
           next_vaddr->next = NULL;
           reg->occupied = 1;
           next_vaddr->occupied = 0;
           if(reg->size == NULL) {
             printk("Block SIZE IS NULL");
             next_vaddr->size = NULL;
           } else {
             printk("Block size is not NULL");
             next_vaddr->size = reg->size - num_pages;
           }
           reg->size = num_pages;
           printk("Allocated size = %lx",reg->size);
           printk("return addr = %lx",reg->addr_start);
           return reg->addr_start;
         } else if(reg->size == num_pages) {
           printk("BLOCK of exact size found");
           return reg->addr_start;
         } else {
           printk("Block size smaller ... Moving to next node");
           reg = reg->next;
         }
      }
      printk("reg->next = %lx",reg->next);
    } while(reg->next != NULL);
    printk("Memory allocation End\n");
    return NULL;
}

void
petmem_dump_vspace(struct mem_map * map)
{
    printk("BR = %lx",map->br);
    printk("MAP START = %lx",map->start);
    printk("MAP END = %lx",map->end);
    struct vaddr_reg * temp = map->start;
    do {
      printk("OCCUPIED = %lx",temp->occupied);
      printk("ADDR_START = %lx",temp->addr_start);
      temp = temp->next;
    }while(temp->next != NULL);
    return;
}




// Only the PML needs to stay, everything else can be freed
void
petmem_free_vspace(struct mem_map * map,
		   uintptr_t        vaddr)
{
    printk("Free Memory\n");
    if(map == NULL || vaddr == NULL) return;
    struct vaddr_reg * node;
    for(node = map->start; node != NULL; node = node->next) {
      if(node->addr_start == vaddr) {
        break;
      }
    }
    if(node == NULL) return;
    free_pagetable(node,vaddr);
    node->occupied = 0;
    if(node->next == NULL) {
      node->prev->next = NULL;
      kfree(node);      
    }
    if(node->next != NULL && node->next->occupied == 0) {
      node->size += node->next->size;
      node->next = node->next->next;
      if(node->next != NULL)
        node->next->prev = node;
      kfree(node->next);
    }
    if(node->prev != NULL && node->prev->occupied == 0) {
      node = node->prev;
      node->size += node->next->size;
      node->next = node->next->next;
      if(node->next != NULL)
        node->next->prev = node;
      kfree(node->next);
    }
    return;
}

void free_pagetable(struct vaddr_reg * node,uintptr_t vaddr) {
  int i,j,pteflag = 1,pdeflag = 1,pdpflag = 1;
  u64 pml_index, pdp_index, pde_index, pte_index;
//  struct vaddr_reg * node = (struct vaddr_reg *) vaddr;
  for(i = 0; i < node->size; i++) {
    pml_index = PML4E64_INDEX(vaddr);
    pdp_index = PDPE64_INDEX(vaddr);
    pde_index = PDE64_INDEX(vaddr);
    pte_index = PTE64_INDEX(vaddr);
    vaddr += PAGE_SIZE_4KB;
    struct pml4e64 * pml4;
    struct pdpe64 * pdp;
    struct pde64 * pde;
    struct pte64 * pte;
    printk("Page free at %lx cr3 %lx pml %lx pdp %lx pd %lx pt %lx\n", vaddr, get_cr3(), pml_index, pdp_index, pde_index, pte_index);
    pml4 = (struct pml4e64 *) ( CR3_TO_PML4E64_VA(get_cr3()) + pml_index * sizeof(struct pml4e64));
    if(pml4->present == 1) {
      pdp = (struct pdpe64 *) (__va(BASE_TO_PAGE_ADDR(pml4->pdp_base_addr)) + pdp_index * sizeof(struct pdpe64));
      if(pdp->present == 1) {
        pde = (struct pde64 *) (__va(BASE_TO_PAGE_ADDR(pdp->pd_base_addr)) + pde_index * sizeof(struct pde64));
        if(pde->present == 1) {
          pte = (struct pte64 *) (__va(BASE_TO_PAGE_ADDR(pde->pt_base_addr)) + pte_index * sizeof(struct pte64));
          if(pte->present == 1) {
            petmem_free_pages(BASE_TO_PAGE_ADDR(pte->page_base_addr),1);
            invlpg(__va(pte->page_base_addr));
            pte->writable = pte->user_page = pte->page_base_addr = pte->present = 0;
          }
          struct pte64 * ptepages = (struct pte64 *)__va(BASE_TO_PAGE_ADDR(pde->pt_base_addr));
          for(j = 0; j < MAX_PTE64_ENTRIES; j++) {
            if(ptepages->present == 1) {
              pteflag = 0;
              break;
            }
            ptepages += 1;
          }
          if(pteflag == 1) {
            petmem_free_pages(BASE_TO_PAGE_ADDR(pde->pt_base_addr),1);
            invlpg(__va(pde->pt_base_addr));
            pde->writable = pde->user_page = pde->pt_base_addr = pde->present = 0;
          }
        }
        struct pde64 * pdepages = (struct pde64 *)__va(BASE_TO_PAGE_ADDR(pdp->pd_base_addr));
        for(j = 0; j < MAX_PDE64_ENTRIES; j++) {
          if(pdepages->present == 1) {
            pdeflag = 0;
            break;
          }
          pdepages += 1;
        }
        if(pdeflag == 1) {
          petmem_free_pages(BASE_TO_PAGE_ADDR(pdp->pd_base_addr),1);
          invlpg(__va(pdp->pd_base_addr));
          pdp->writable = pdp->user_page = pdp->pd_base_addr = pdp->present = 0;
        }
      }
      struct pdpe64 * pdppages = (struct pdpe64 *)__va(BASE_TO_PAGE_ADDR(pml4->pdp_base_addr));
      for(j = 0; j < MAX_PDPE64_ENTRIES; j++) {
        if(pdppages->present == 1) {
          pdpflag = 0;
          break;
        }
        pdppages += 1;
      }
      if(pdpflag == 1) {
        petmem_free_pages(BASE_TO_PAGE_ADDR(pml4->pdp_base_addr),1);
        invlpg(__va(pml4->pdp_base_addr));
        pml4->writable = pml4->user_page = pml4->pdp_base_addr = pml4->present = 0;
      }
    }  
  }
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
    printk("map = %lx",map);
    printk("fault_addr = %lx",fault_addr);
    printk("error code = %lx",error_code);
    if(map == NULL) return -1;
    struct vaddr_reg * node;
    for(node = map->start; node != NULL; node = node->next) {
      if(fault_addr > node->addr_start && fault_addr < (node->addr_start + node->size * PAGE_SIZE) && node->occupied == 1) {
        break;
      }
    }
    if(node == NULL ) return -1;
    if(error_code == 2) return -1;
    if(error_code == 1) {
      printk("Inside error 1");
      struct pml4e64 * pml4;
      struct pdpe64 * pdp;
      struct pde64 * pde;
      struct pte64 * pte;
      u64 pml_index, pdp_index, pde_index, pte_index;
      pml_index = PML4E64_INDEX(fault_addr);
      pdp_index = PDPE64_INDEX(fault_addr);
      pde_index = PDE64_INDEX(fault_addr);
      pte_index = PTE64_INDEX(fault_addr);
      printk("Page fault at %lx err %d cr3 %lx pml %lx pdp %lx pd %lx pt %lx\n", fault_addr, error_code, get_cr3(), pml_index, pdp_index, pde_index, pte_index);
      pml4 = (struct pml4e64 *) ( CR3_TO_PML4E64_VA(get_cr3()) + pml_index * sizeof(struct pml4e64));
      if(pml4->present == 0) {
         pml4->present = 1;
         pml4->writable = 1;
         pml4->user_page = 1;
         pml4->pdp_base_addr = PAGE_TO_BASE_ADDR(petmem_alloc_pages(1));
         memset(__va(pml4->pdp_base_addr),0,PAGE_SIZE_4KB);
      }
      pdp = (struct pdpe64 *) (__va(BASE_TO_PAGE_ADDR(pml4->pdp_base_addr)) + pdp_index * sizeof(struct pdpe64));
      if(pdp->present == 0) {
         pdp->present = 1;
         pdp->writable = 1;
         pdp->user_page = 1;
         pdp->pd_base_addr = PAGE_TO_BASE_ADDR(petmem_alloc_pages(1));
         memset(__va(pdp->pd_base_addr),0,PAGE_SIZE_4KB);
      }
      pde = (struct pde64 *) (__va(BASE_TO_PAGE_ADDR(pdp->pd_base_addr)) + pde_index * sizeof(struct pde64));
      if(pde->present == 0) {
         pde->present = 1;
         pde->writable = 1;
         pde->user_page = 1;
         pde->pt_base_addr = PAGE_TO_BASE_ADDR(petmem_alloc_pages(1));
         memset(__va(pde->pt_base_addr),0,PAGE_SIZE_4KB);
      }
      pte = (struct pte64 *) (__va(BASE_TO_PAGE_ADDR(pde->pt_base_addr)) + pte_index * sizeof(struct pte64));
      if(pte->present == 0) {
         pte->present = 1;
         pte->writable = 1;
         pte->user_page = 1;
         pte->page_base_addr = PAGE_TO_BASE_ADDR(petmem_alloc_pages(1));
         memset(__va(pte->page_base_addr),0,PAGE_SIZE_4KB);
      }
      printk("return success");
      return 0;
    }
    return -1;
}
