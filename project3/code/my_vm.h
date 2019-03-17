#ifndef MY_VM_H_INCLUDED
#define MY_VM_H_INCLUDED
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <sys/mman.h>
#include <math.h>

//Assume the address space is 32 bits, so the max memory size is 4GB
//Page size is 4KB

//Add any important includes here which you may need

#define PGSIZE (4096)
#define MAX_MEMSIZE (4*1024*1024*1024)
#define MEMSIZE (2.0*1024*1024*1024)
#define PAGETABLEENTRYSIZE (sizeof(pte_t))
// #define TLB_SIZE



typedef unsigned long pte_t;
typedef unsigned long pde_t;

// unsigned short pageTableEntrySize = 4;
char* physical_mem;
unsigned int numPages;
unsigned int numDirEntries;
unsigned int numTableEntries;
int numPagesBits;
int numOffsetBits;
int numPageDirBits;
int numPageTableBits;
unsigned int lower_bitmask;
unsigned int middle_bitmask;
unsigned int upper_bitmask;
pde_t* page_dir;
int* bitmap;

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
int log_2(int x);
unsigned int getPageOffset(void* va);
unsigned int getTableIndex(void* va);
unsigned int getDirIndex(void* va);
void setBit(int);
void clearBit( int);
int testBit( int);
#endif
