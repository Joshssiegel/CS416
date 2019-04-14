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

#define PGSIZE (4096*16) //MUST BE POWER OF TWO
#define MAX_MEMSIZE (3*1024*1024*1024+1024*1024*500)//758 causes seg fault
#define MEMSIZE (3*1024*1024*1024+1024*1024*500)//758 causes seg fault
#define PAGETABLEENTRYSIZE (sizeof(pte_t))
#define TLB_SIZE 16   //number of TLB entries
#define OFFSET 32 // offset for Virtual Address



typedef unsigned long pte_t;
typedef unsigned long pde_t;

// unsigned short pageTableEntrySize = 4;
unsigned int virt_page_size;
char* physical_mem;
unsigned int numPages;
unsigned int numDirEntries;
unsigned int numTableEntries;
int numTotalBits;
int numPagesBits;
int numOffsetBits;
int numPageDirBits;
int numPageTableBits;
int numTLBBits;
unsigned int lower_bitmask;
unsigned int middle_bitmask;
unsigned int upper_bitmask;
unsigned int tlb_bitmask;
pde_t* page_dir;
int* bitmap;


struct tlb {
    //fill this in. this structure will represent a tlb.
    //assume it is a direct mapped TLB of TLB_SIZE (buckets)
    //assume each bucket to be 4 bytes
    pte_t* translations;
    int* virtual_tags;
    double hits;
    double misses;

};

struct tlb* tlb_store;



void set_physical_mem();
pte_t* translate(pde_t *pgdir, void *va);
int page_map(pde_t *pgdir, void *va, void* pa);
int page_unmap(pde_t *pgdir, void *va);

bool check_in_tlb(void *va);
void put_in_tlb(void *va, void *pa);
void *a_malloc(unsigned int num_bytes);
void a_free(void *va, unsigned int size);
void put_value(void *va, void *val, int size);
void get_value(void *va, void *val, int size);
void mat_mult(void *mat1, void *mat2, int size, void *answer);
int log_2(int x);
unsigned int getPageOffset(void* va);
unsigned int getTableIndex(void* va);
unsigned int getDirIndex(void* va);
unsigned int getTLBIndex(void* va);
pte_t* searchTLB(pte_t* va);
int checkAllocated(void *va, int size);
void setBit(int);
void clearBit( int);
int testBit( int);
#endif
