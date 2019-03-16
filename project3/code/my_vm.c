#include "my_vm.h"

int log_2(int x){
  unsigned int ans = 0 ;
  while( x>>=1 ) {
    ans++;
  }
  return ans ;
}
void set_physical_mem() {
    //allocate physical memory using mmap or malloc
    physical_mem =(char*) mmap(NULL, MEMSIZE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANON, -1, 0);
    //Calculate bits needed and create bitmasks needed for translation
    unsigned int num_pages=(MEMSIZE)/(PGSIZE);
    printf("numPages: %d\n",num_pages);
    numPagesBits=log2(num_pages);
    numOffsetBits = log2(PGSIZE);
    printf("numOffsetBits: %d\n",numOffsetBits);
    numPageDirBits = numPagesBits/2; //Floor division
    numPageTableBits = numPagesBits - numPageDirBits;
    printf("numPageTableBits: %d\n",numPageTableBits);
    printf("numPageDirBits: %d\n",numPageDirBits);
    lower_bitmask= (int) pow(2,numOffsetBits)-1;
    middle_bitmask= (int) (pow(2, numPageTableBits)-1 ) << numOffsetBits;
    upper_bitmask=(int) (pow(2, numPageDirBits)-1 ) << (numOffsetBits+numPageTableBits);
    printf("lower bitmask: 0x%X\n",lower_bitmask);
    printf("middle bitmask: 0x%X\n",middle_bitmask);
    printf("upper bitmask: 0x%X\n",upper_bitmask);

}

pte_t * translate(pde_t *pgdir, void *va)
{
    //you are given a page directory pointer and a virtual address
    //walk the page directory to get the address of the second level page table
    //then use the second level page directory and offset in virtual address to return the
    //physical address

    //page offset is taken from the LSB of the va
    int page_offset = ((int)va)&lower_bitmask;
    //get the index of the page table
    int page_table_index = (((int)va)&middle_bitmask) >> numOffsetBits;
    //get index of page directory;
    int page_directory_index = (((int)va)&upper_bitmask) >> (numOffsetBits+numPageTableBits);
    printf("virtual addr: 0x%X\n",va);
    printf("page_offset: 0x%X\n ",page_offset);
    printf("page_table_index: 0x%X\n ",page_table_index);
    printf("page_dir_index: 0x%X\n ",page_directory_index);
    // now we go into page dir, to get the page table for that entry
    int *addrOfPageDirEntry = pgdir + page_directory_index*PAGETABLEENTRYSIZE;
    if(*addrOfPageDirEntry==0){
      printf("directory entry unallocated\n");
      return NULL;
    }
    int *addrOfPageTable = *addrOfPageDirEntry;
    int *addrOfPageTableEntry = addrOfPageTable + page_table_index*PAGETABLEENTRYSIZE;
    if(*addrOfPageDirEntry==0){
      printf("table entry unallocated\n");
      return NULL;
    }
    int *physicalPageAddr = *addrOfPageTableEntry;

    // return physicalPageAddr;

    return NULL;
}

int page_map(pde_t *pgdir, void *va, void *pa)
{
    //walk the page directory to see if the virtual address is present or not
    //if not store the entry
    //you might have to reserve some extra space for page table if its not already allocated
    return -1;
}

void *a_malloc(unsigned int num_bytes) {
    //you will allocate the pages required to store the given number of bytes as
    //continuous virtual address.
    //you will have to store the page table entries
    //you will also have to mark which physical pages have been used
    return NULL;
}

void a_free(void *va, int size) {
    //free the page table entries starting from this virtual address and uptill size
    //mark the pages which you recently free
    //only free if the memory from va till va+size is valid
}

void put_value(void *va, void *val, int size) {
    //put the values pointed to by val inside the physical memory at given virtual address
    //assume you can access val address directly by derefencing it
    //Also, it has the capability to put values inside the tlb
    //just implement a simple first in first out scheme for value eviction
}

void get_value(void *va, void *val, int size) {
    //put the values pointed to by va inside the physical memory at given val address
    //assume you can access val address directly by derefencing them
    //always check first the presence of translation inside the tlb before proceeding forward

    //for testing purpose:
}

void mat_mult(void *mat1, void *mat2, int size, void *answer) {
    //given two arrays of length: size * size
    //multiply them as matrices and store the computed result in answer
    set_physical_mem();
    translate(NULL, physical_mem[0]);

   //Hint: You will do indexing as [i * size + j] where i, j are the indices of matrix being accessed
}
