#ifndef MY_VM_H
#define MY_VM_H
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//Assume the address space is 32 bits, so the max memory size is 4GB
//Page size is 4KB

//Add any important includes here which you may need

#define PGSIZE 4096
#define MAX_MEMSIZE 4294967295//4*1024*1024*1024
#define MEMSIZE 1024*1024*1024

// unsigned long MAX_MEMSIZE = (4*1024*1024*1024);

// #define PDX(va)         (((uint)(va) >> PDXSHIFT) & 0x3FF)

typedef unsigned int pte_t;
typedef unsigned int pde_t;

//this marks the start of the physical memory
void *phys_memory;
char *phys_bitmap;
char *virt_bitmap;

// int virt_start = 0;
pde_t *root_dir = NULL;

struct tlb {
    //file this in. this structure will represent a tlb.
    //assume it is a 4 way associative TLB with 4 sets (buckets)
    //assume each bucket to be 4 bytes
};

struct tlb tlb_store;

void set_physical_mem();
pte_t* translate(pde_t *pgdir, void *va);
int mappage(pde_t *pgdir, void *va, void* pa);
bool check_in_tlb(void *va);
void put_in_tlb(void *va, void *pa);
void *a_malloc(unsigned int num_bytes);
void a_free(void *va, int size);
void put_value(void *va, void *val, int size);
void get_value(void *va, void *val, int size);
void mat_mult(void *mat1, void *mat2, int size, void *answer);

#endif
