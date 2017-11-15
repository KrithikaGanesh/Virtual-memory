
#include <linux/module.h>

#define MAX_PML4E64_ENTRIES        512
#define MAX_PDPE64_ENTRIES         512
#define MAX_PDE64_ENTRIES          512
#define MAX_PTE64_ENTRIES          512


/* Converts an address into a page table index */
#define PML4E64_INDEX(x) ((((u64)x) >> 39) & 0x1ff)
#define PDPE64_INDEX(x)  ((((u64)x) >> 30) & 0x1ff)
#define PDE64_INDEX(x)   ((((u64)x) >> 21) & 0x1ff)
#define PTE64_INDEX(x)   ((((u64)x) >> 12) & 0x1ff)


/* Gets the base address needed for a Page Table entry */
#define PAGE_TO_BASE_ADDR(x)     ((x) >> 12)

#define BASE_TO_PAGE_ADDR(x)     (((uintptr_t)x) << 12)

/* *** */


//#define PAGE_OFFSET(x) ((x) & 0xfff)
#define PAGE_OFFSET_4KB(x) ((x) & 0xfff)

#define PAGE_POWER     12


// We shift instead of mask because we don't know the address size
#define PAGE_ADDR(x)     (((x) >> PAGE_POWER) << PAGE_POWER)

//#define PAGE_SIZE 4096
#define PAGE_SIZE_4KB (4096)


/* *** */


#define CR3_TO_PML4E64_PA(cr3)   ((uintptr_t)(((u64)cr3) & 0x000ffffffffff000LL))
    
#define CR3_TO_PML4E64_VA(cr3)   ((pml4e64_t *)__va((void *)(uintptr_t)(((u64)cr3) & 0x000ffffffffff000LL)))



typedef struct gen_pt {
    u32 present         : 1;
    u32 writable        : 1;
    u32 user_page       : 1;
} __attribute__((packed)) gen_pt_t;



/* LONG MODE 64 bit PAGE STRUCTURES */
typedef struct pml4e64 {
    u64 present        : 1;
    u64 writable       : 1;
    u64 user_page      : 1;
    u64 write_through  : 1;
    u64 cache_disable  : 1;
    u64 accessed       : 1;
    u64 reserved       : 1;
    u64 zero           : 2;
    u64 vmm_info       : 3;
    u64 pdp_base_addr  : 40;
    u64 available      : 11;
    u64 no_execute     : 1;
} __attribute__((packed)) pml4e64_t;


typedef struct pdpe64 {
    u64 present        : 1;
    u64 writable       : 1;
    u64 user_page      : 1;
    u64 write_through  : 1;
    u64 cache_disable  : 1;
    u64 accessed       : 1;
    u64 avail          : 1;
    u64 large_page     : 1;
    u64 zero           : 1;
    u64 vmm_info       : 3;
    u64 pd_base_addr   : 40;
    u64 available      : 11;
    u64 no_execute     : 1;
} __attribute__((packed)) pdpe64_t;


typedef struct pde64 {
    u64 present        : 1;
    u64 writable       : 1;
    u64 user_page      : 1;
    u64 write_through  : 1;
    u64 cache_disable  : 1;
    u64 accessed       : 1;
    u64 avail          : 1;
    u64 large_page     : 1;
    u64 global_page    : 1;
    u64 vmm_info       : 3;
    u64 pt_base_addr   : 40;
    u64 available      : 11;
    u64 no_execute     : 1;
} __attribute__((packed)) pde64_t;

typedef struct pte64 {
    u64 present        : 1;
    u64 writable       : 1;
    u64 user_page      : 1;
    u64 write_through  : 1;
    u64 cache_disable  : 1;
    u64 accessed       : 1;
    u64 dirty          : 1;
    u64 pte_attr       : 1;
    u64 global_page    : 1;
    u64 vmm_info       : 3;
    u64 page_base_addr : 40;
    u64 available      : 11;
    u64 no_execute     : 1;
} __attribute__((packed)) pte64_t;

/* *************** */

typedef struct pf_error_code {
    u32 present        : 1; // if 0, fault due to page not present
    u32 write          : 1; // if 1, faulting access was a write
    u32 user           : 1; // if 1, faulting access was in user mode
    u32 rsvd_access    : 1; // if 1, fault from reading a 1 from a reserved field (?)
    u32 ifetch         : 1; // if 1, faulting access was an instr fetch (only with NX)
    u32 rsvd           : 27;
} __attribute__((packed)) pf_error_t;
