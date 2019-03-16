#ifndef MY_VM_H_INCLUDED
#define MY_VM_H_INCLUDED
#include <stdbool.h>
#include <stdlib.h> 

//Assume the address space is 32 bits, so the max memory size is 4GB
//Page size is 4KB

//Add any important includes here which you may need

#define PGSIZE 4096
#define MAX_MEMSIZE 4*1024*1024*1024
#define MEMSIZE 1024*1024*1024
// #define TLB_SIZE

typedef unsigned long pte_t;
typedef unsigned long pde_t;

char* physical_mem;

struct tlb {
    //file this in. this structure will represent a tlb.
    //assume it is a direct mapped TLB of TBL_SIZE (buckets)
    //assume each bucket to be 4 bytes
};

struct tlb tlb_store;

void set_physical_mem();
pte_t* translate(pde_t *pgdir, void *va);
int page_map(pde_t *pgdir, void *va, void* pa);
bool check_in_tlb(void *va);
void put_in_tlb(void *va, void *pa);
void *a_malloc(unsigned int num_bytes);
void a_free(void *va, int size);
void put_value(void *va, void *val, int size);
void get_value(void *va, void *val, int size);
void mat_mult(void *mat1, void *mat2, int size, void *answer);

#endif
