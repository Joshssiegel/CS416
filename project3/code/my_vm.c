#include "my_vm.h"

int log_2(int x){
  unsigned int ans = 0 ;
  while( x>>=1 ) {
    ans++;
  }
  return ans ;
}
int getOptimalVacantPages(int pagesToAllocate){//returns index of starting page
  // bitmap
  int i = 0;
  int counter = 0;
  int leastRegionFound = numPages;
  int index = -1;

  for(i=0; i<numPages; i++){
    if(testBit(i)==0){ // not set
      counter++;
    }
    else{
      if(counter == pagesToAllocate){
        index = i - counter;//FOUND OPTIMAL SPOT
        return index;
      }
      if(counter>pagesToAllocate && counter<=leastRegionFound){
        leastRegionFound = counter;
        index = i - counter;
      }
      counter = 0;
    }
  }
  return index;
}

void setBit(int bit){
  bitmap[(bit/32)] |= (1 << (bit%32));
}
void clearBit(int bit){
  bitmap[(bit/32)] &= ~(1 << (bit%32));
}
int testBit(int bit){
  return ( (bitmap[bit/32] & (1 << (bit%32) )) != 0 );
}
unsigned int getPageOffset(void* va){
  unsigned int page_offset = ((int)va)&lower_bitmask;
  return page_offset;
}
unsigned int getTableIndex(void* va){
  unsigned int page_table_index = (((int)va)&middle_bitmask) >> numOffsetBits;
  return page_table_index;
}
unsigned int getDirIndex(void* va){
  unsigned int page_directory_index = (((int)va)&upper_bitmask) >> (numOffsetBits+numPageTableBits);
  return page_directory_index;
}

void set_physical_mem() {
    //allocate physical memory using mmap or malloc
    physical_mem =(char*) mmap(NULL, MEMSIZE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANON, -1, 0);
    if(physical_mem==-1){
      printf("allocating memory failed\n");
      exit(-1);
    }
    //Calculate bits needed and create bitmasks needed for translation
    numPages=(MEMSIZE)/(PGSIZE);
    printf("numPages: %d\n",numPages);
    numPagesBits=log2(numPages);
    numOffsetBits = log2(PGSIZE);
    printf("numOffsetBits: %d\n",numOffsetBits);
    numPageDirBits = numPagesBits/2; //Floor division
    numPageTableBits = numPagesBits - numPageDirBits;
    printf("numPageTableBits: %d\n",numPageTableBits);
    printf("numPageDirBits: %d\n",numPageDirBits);
    numDirEntries=pow(2,numPageDirBits);
    numTableEntries=pow(2,numPageTableBits);
    lower_bitmask= (int) pow(2,numOffsetBits)-1;
    middle_bitmask= (int) (pow(2, numPageTableBits)-1 ) << numOffsetBits;
    upper_bitmask=(int) (pow(2, numPageDirBits)-1 ) << (numOffsetBits+numPageTableBits);
    printf("lower bitmask: 0x%X\n",lower_bitmask);
    printf("middle bitmask: 0x%X\n",middle_bitmask);
    printf("upper bitmask: 0x%X\n",upper_bitmask);
    //initialize page directory to point to 2^(numbits) entries
    page_dir=(pde_t*) malloc(numDirEntries*PAGETABLEENTRYSIZE);
    bitmap=(int*) calloc(numPages/32,sizeof(int));

}

pte_t * translate(pde_t *pgdir, void *va)
{
    //you are given a page directory pointer and a virtual address
    //walk the page directory to get the address of the second level page table
    //then use the second level page directory and offset in virtual address to return the
    //physical address

    //page offset is taken from the LSB of the va
    unsigned int page_offset = getPageOffset(va);
    //get the index of the page table
    unsigned int page_table_index = getTableIndex(va);
    //get index of page directory;
    unsigned int page_directory_index = getDirIndex(va);
    printf("virtual addr: 0x%X\n",va);
    printf("page_offset: 0x%X\n ",page_offset);
    printf("page_table_index: 0x%X\n ",page_table_index);
    printf("page_dir_index: 0x%X\n ",page_directory_index);
    // now we go into page dir, to get the page table for that entry
    pde_t *addrOfPageDirEntry = pgdir + page_directory_index;
    if(*addrOfPageDirEntry==0){
      printf("directory entry unallocated, returning NULL\n");
      return NULL;
    }
    pte_t *addrOfPageTable = *addrOfPageDirEntry;
    pte_t *addrOfPageTableEntry = addrOfPageTable + page_table_index;
    if(*addrOfPageDirEntry==0){
      printf("table entry unallocated, exiting\n");
      return NULL;
    }
    pte_t *physicalPageAddr = *addrOfPageTableEntry + page_offset;

    return physicalPageAddr;

    //return NULL;
}

int page_map(pde_t *pgdir, void *va, void *pa)
{
    //walk the page directory to see if the virtual address is present or not
    //if not store the entry
    //you might have to reserve some extra space for page table if its not already allocated
    if(pgdir == NULL){
      printf("page directory not set, returning -1\n");
      return -1;
    }
    //extract the directory index from the address
    unsigned int directory_index=getDirIndex(va);

    //if the table at the index has not been initialized
    if(pgdir[directory_index]==NULL){
      //create the page table;
      pgdir[directory_index]=(pte_t*) malloc(numTableEntries*PAGETABLEENTRYSIZE);
    }
    //get the beginning of the inner page table
    pte_t* page_table=pgdir[directory_index];
    //get the index of the inner page table
    unsigned int table_index=getTableIndex(va);
    //if this page table entry isn't mapped to anything yet, map it to the physical addr
    if(page_table[table_index]==NULL){
      page_table[table_index]=pa;
    }
    return -1;
}

void* a_malloc(unsigned int num_bytes) {
    //you will allocate the pages required to store the given number of bytes as
    //continuous virtual address.
    //you will have to store the page table entries
    //you will also have to mark which physical pages have been used
    // Step 1) Check if page directory has been initialized, if not, call set_physical_mem()
    if(page_dir==NULL){
      set_physical_mem();
    }
    // Step 2) Convert num_bytes to allocate into numPages to allocate
    unsigned int pages_to_allocate=num_bytes/PGSIZE;
    // Step 3) Get Shortest Continuous Memory Region
    int pageIndex=getOptimalVacantPages(pages_to_allocate);
    if(pageIndex==-1){
      printf("No space to allocate. Returning NULL\n");
      return NULL;
    }
    // Step 4) Allocate pages found by setting their value to “1” in the bitmap.
    int i=0;
    for(i=0;i<pages_to_allocate;i++){
      setBit(pageIndex+i);
    }
    // Step 5) Choose the virtual addr and map it to the physical pages.
    //Virtual address = 0 + PGSIZE*INDEX_OF_ALLOCATED_PAGE
    void* va=PGSIZE*pageIndex;
    // Step 6) return virtual addr of first page in this contiguous block to user.
    return va;
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
    int i=0;
    setBit(0);
    setBit(31);
    setBit(1024);
    printf("bitmap first val is: 0x%X\n",bitmap[0]);
    printf("Testing bit 1023: 0x%X\n", testBit(1024));



   //Hint: You will do indexing as [i * size + j] where i, j are the indices of matrix being accessed
}
