#include "my_vm.h"
pthread_mutex_t lock;

int log_2(int x){
  unsigned int ans = 0 ;
  while( x>>=1 ) {
    ans++;
  }
  return ans ;
}
void *unoffset (void *va){
  if(va<20){
    printf("Bad Address. Cannot process. Returning -1\n");
    return -1;
  }
  unsigned int new_va = (unsigned int)va - OFFSET;
  return (void*)new_va;
}
int checkAllocated(void *va, int size){
  if(va<0){
    printf("!!! Bad Virtual address: 0x%X\n",va);

    return -1;
  }
  int counter=0;
  pte_t** pa=malloc( (size/PGSIZE+2)*sizeof(pte_t*)  );
  //count pages
  do{
    pa[counter]=translate(page_dir, va+PGSIZE*counter);
    // printf("found translation virtual 0x%x to physical 0x%x\n",va+PGSIZE*counter,pa[counter] );
    if(pa[counter]==NULL){
      printf("!!! Virtual address: 0x%X was not allocated.\n",va+PGSIZE*counter);
      free(pa);
      return -1;
    }
    counter+=1;
    size-=PGSIZE;
  }while(size>0);
  pthread_mutex_lock(&lock);
  int i=0;
  int pageNums[counter];
  for(i=0;i<counter;i++){
    int offset=getPageOffset(va+i*PGSIZE);
    //printf("difference from base%d\n",(int)pa[i]);
    pageNums[i]=((int)pa[i]-offset-(int)physical_mem)/PGSIZE;
    //printf("testing page number: %d\n",pageNums[i]);
    if(testBit(pageNums[i])==0){
      printf("!!! Page %d was not allocated.\n", pageNums[i]);
      pthread_mutex_unlock(&lock);
      free(pa);
      return-1;
    }
  }
  free(pa);
  pthread_mutex_unlock(&lock);
  return 0;
}
int getOptimalVacantPages(int pagesToAllocate){//returns index of starting page
  // bitmap
  int i = 0;
  int counter = 0;
  int leastRegionFound = numPages+2;
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
      if(counter>=pagesToAllocate && counter<leastRegionFound){
        leastRegionFound = counter;
        index = i - counter;
      }
      counter = 0;
    }
  }
  //last place is best
  if(counter>=pagesToAllocate && counter<leastRegionFound){
    index = i - counter;
    printf("last place best\n");
  }
  printf("pages to allocate: %d, i is: %d counter is %d and least region found is: %d\n",pagesToAllocate,i, counter,leastRegionFound);
  printf("index to alocate pages %d\n",index);
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

unsigned int getTLBIndex(void* va){
  unsigned int tlb_index = (((int)va)&tlb_bitmask) >> (numOffsetBits);
  return tlb_index;
}

void set_physical_mem() {
    //allocate physical memory using mmap or malloc
    unsigned long mem_size=MEMSIZE;
    unsigned long max_mem_size=MAX_MEMSIZE;
    if(mem_size>max_mem_size || mem_size<0){
      mem_size=MAX_MEMSIZE;
       printf("asked to allocate %u so setting it to Max=%u\n",mem_size,max_mem_size);
    }
    else{
      // mem_size=MEMSIZE;
      printf("physical mem size =%u < max=%u\n",mem_size, max_mem_size);
    }
    physical_mem =(char*) mmap(NULL, mem_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANON,  -1, 0);
    // physical_mem =(char*) mmap(NULL, MEMSIZE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANON,  -1, 0);
    if((int)physical_mem==-1){
      printf("allocating memory failed\n");
      perror("error: ");
      exit(-1);
    }
    printf("physical mem located at: 0x%X\n",physical_mem);
    //Calculate bits (needed and create bitmasks needed for translation
    numTotalBits=(int)ceil(log2(mem_size));
    printf("total bits being used: %d\n",numTotalBits);
    numPages=(int)ceil((mem_size)/(PGSIZE));
    printf("num pages: %d\n",numPages);
    numPagesBits=(int)ceil(log2(numPages));
    numOffsetBits = (int)ceil(log2(PGSIZE));
    numPageDirBits = numPagesBits/2; //Floor division
    numPageTableBits = numPagesBits - numPageDirBits;
    numTLBBits= (int)ceil(log2(TLB_SIZE));
    numDirEntries=pow(2,numPageDirBits);
    numTableEntries=pow(2,numPageTableBits);
    lower_bitmask= (int) pow(2,numOffsetBits)-1;
    middle_bitmask= (int) (pow(2, numPageTableBits)-1 ) << numOffsetBits;
    upper_bitmask=(int) (pow(2, numPageDirBits)-1 ) << (numOffsetBits+numPageTableBits);
    tlb_bitmask=(int) (pow(2,numTLBBits)-1)<<(numOffsetBits);
    printf("tlb bitmask: 0x%X\n",tlb_bitmask);
    //initialize page directory to point to 2^(numbits) entries
    page_dir=(pde_t*) calloc(numDirEntries,PAGETABLEENTRYSIZE);
    int numEntriesInBitmap=numPages%32==0?numPages/32: (numPages/32)+1;
    bitmap=(int*) calloc(numEntriesInBitmap,sizeof(int));
    printf("bitmap initialized to size: %d\n", (numPages/32));
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("!!! Failed to initialize mutex\n");
        return;
    }
    //initialize TLB
    tlb_store=(struct tlb*) malloc(sizeof(struct tlb));
    tlb_store->translations=(pte_t*) calloc(TLB_SIZE,sizeof(pte_t));
    tlb_store->virtual_tags=(int*) calloc(TLB_SIZE,sizeof(int));
    tlb_store->hits=0;
    tlb_store->misses=0;
    int i=0;
    for (i=0;i<TLB_SIZE;i++){
      tlb_store->virtual_tags[i]=-1;
    }
    printf("done set physical mem\n");
}

pte_t * translate(pde_t *pgdir, void *va)
{
    //you are given a page directory pointer and a virtual address
    //walk the page directory to get the address of the second level page table
    //then use the second level page directory and offset in virtual address to return the
    //physical address
    //page offset is taken from the LSB of the va

    // printf("***Translating address: 0x%x\n", va);

    unsigned int page_offset = getPageOffset(va);
    //get the index of the page table
    unsigned int page_table_index = getTableIndex(va);
    //get index of page directory;
    unsigned int page_directory_index = getDirIndex(va);
    // printf("page offset: 0x%X\n",page_offset);
    // printf("page table index: 0x%X\n",page_table_index);
    // printf("page directory index: 0x%X\n",page_directory_index);
    pde_t *addrOfPageDirEntry = pgdir + page_directory_index;
    pthread_mutex_lock(&lock);
    if(*addrOfPageDirEntry==NULL){
      printf("directory entry unallocated, returning NULL\n");
      pthread_mutex_unlock(&lock);
      return NULL;
    }
    pte_t *addrOfPageTable = (pte_t*)   *addrOfPageDirEntry;
    pte_t *addrOfPageTableEntry = addrOfPageTable + page_table_index;
    if(*addrOfPageTableEntry==NULL){
      printf("table entry unallocated, exiting\n");
      pthread_mutex_unlock(&lock);
      return NULL;
    }

    pte_t *physicalPageAddr = (pte_t*)(*addrOfPageTableEntry + page_offset);//can do this because *addrOfPageTableEntry is an unsigned long
    pthread_mutex_unlock(&lock);
    return physicalPageAddr;
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
    //pthread_mutex_lock(&lock);

    //if the table at the index has not been initialized
    if(pgdir[directory_index]==0){
      //create the page table;
      pgdir[directory_index]=(pde_t) calloc(numTableEntries,PAGETABLEENTRYSIZE);
    }
    //get the beginning of the inner page table
    pte_t* page_table=(pte_t*)pgdir[directory_index];
    //get the index of the inner page table
    unsigned int table_index=getTableIndex(va);
    //if this page table entry isn't mapped to anything yet, map it to the physical addr
    if(page_table[table_index]==0){
      page_table[table_index]=(pte_t)pa;
    }
    //pthread_mutex_unlock(&lock);

    return 0;
}

int page_unmap(pde_t *pgdir, void *va)
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
    //pthread_mutex_lock(&lock);

    //if the table at the index has not been initialized
    if(pgdir[directory_index]==0){
      //create the page table;
      printf("!!! Can't unmap an unallocated memory address, returning -1\n");
      // pthread_mutex_unlock(&lock);
      return -1;
    }
    //get the beginning of the inner page table
    pte_t* page_table=(pte_t*)pgdir[directory_index];
    //get the index of the inner page table
    unsigned int table_index=getTableIndex(va);
    //if this page table entry isn't mapped to anything yet, map it to the physical addr
    if(page_table[table_index]==(pte_t)NULL){
      printf("!!! Can't unmap an unallocated memory address, returning -1\n");
      // pthread_mutex_unlock(&lock);
      return -1;
    }
    page_table[table_index]=(pte_t)NULL;
    //remove from TLB if present in it
    // printf("BEFORE:\n");
    // printTLB();
    removeFromTLB(va);
    // printf("AFTER:\n");
    // printTLB();
    // pthread_mutex_unlock(&lock);
    return 0;
}

void removeFromTLB(void *va){

  unsigned int tlb_index = getTLBIndex(va);
  unsigned int offset = getPageOffset(va);
  if(tlb_store->virtual_tags[tlb_index] == (unsigned int)va-offset){// this page was in the TLB
    // printf("Freeing va ==> 0x%x\n This page starts at ==> 0x%x\n", va, (unsigned int)va - offset);
    tlb_store->virtual_tags[tlb_index] = -1;//(unsigned int)NULL;
    tlb_store->translations[tlb_index] = (unsigned int)NULL;
  }
}

void* a_malloc(unsigned int num_bytes) {
    //you will allocate the pages required to store the given number of bytes as
    //continuous virtual address.
    //you will have to store the page table entries
    //you will also have to mark which physical pages have been used
    // Step 1) Check if page directory has been initialized, if not, call set_physical_mem()
    pthread_mutex_lock(&lock);

    if(page_dir==NULL){
      set_physical_mem();
    }
    // Step 2) Convert num_bytes to allocate into numPages to allocate
    unsigned int pages_to_allocate=ceil(num_bytes/PGSIZE);
    // printf("Lock!\n" );

    // Step 3) Get Shortest Continuous Memory Region
    int pageIndex=getOptimalVacantPages(pages_to_allocate);

    if(pageIndex==-1){
      printf("!!! No space to allocate. Returning NULL\n");
      pthread_mutex_unlock(&lock);
      return NULL;
    }
    // Step 4) Allocate pages found by setting their value to “1” in the bitmap.
    int i=0;
    for(i=0;i<pages_to_allocate;i++){
      setBit(pageIndex+i);
    }
    // Step 5) Choose the virtual addr and map it to the physical pages.
    //Virtual address = OFFSET + PGSIZE*INDEX_OF_ALLOCATED_PAGE
    void* va=(void*) (PGSIZE*pageIndex + OFFSET);
    //Physical Page to allocate
    void* pa=physical_mem+PGSIZE*pageIndex;
    for(i=0;i<pages_to_allocate;i++){
      int status=page_map(page_dir, va+PGSIZE*i, pa+PGSIZE*i);

      if(status==-1){
        printf("!!! Error mapping page. Returning NULL \n");
        pthread_mutex_unlock(&lock);
        return NULL;
      }
    }
    // Step 6) return virtual addr of first page in this contiguous block to user.
    pthread_mutex_unlock(&lock);

    return va;
}

void a_free(void *va, int size) {
    //free the page table entries starting from this virtual address and uptill size
    //mark the pages which you recently free
    //only free if the memory from va till va+size is valid

    va = unoffset(va);
    if(va==-1){
      printf("!!! Virtual address is bad. Returning and not freeing.\n");
      return;
    }

    pte_t** pa=malloc( (size/PGSIZE+2)*sizeof(pte_t*)  );
    int counter=0;
    do{
      pa[counter]=translate(page_dir, va+PGSIZE*counter);
      if(pa[counter]==NULL){
        printf("!!! Virtual address: 0x%X was not allocated. Returning and not freeing.\n",va+PGSIZE*counter);
        return;
      }
      counter+=1;
      size-=PGSIZE;
    }while(size>0);
    // printf("asking to free %d pages\n",counter);
    pthread_mutex_lock(&lock);
    int i=0;
    int page_nums_to_free[counter];
    for(i=0;i<counter;i++){
      int offset=getPageOffset(va+i*PGSIZE);
      page_nums_to_free[i]=((int)pa[i]-offset-(int)physical_mem)/PGSIZE;
      if(testBit(page_nums_to_free[i])==0){
        //should never get here
        printf("!!! Page has a translation but was not allocated. Weird. Returning.\n");
        pthread_mutex_unlock(&lock);
        free(pa);
        return;
      }
    }
    free(pa);
    //We are guaranteed that there was a translation and that the pages were allocated
    //Unmap the translations and mark them as unallocated in the bitmap
    for(i=0;i<counter;i++){
      int status=page_unmap(page_dir,va+i*PGSIZE);
      if(status==-1){
        printf("!!! Tried to unmap unallocated page. Returning\n");
        pthread_mutex_unlock(&lock);
        return;
      }
      // printf("freed\n");
      clearBit(page_nums_to_free[i]);

    }
    pthread_mutex_unlock(&lock);
}

void put_value(void *va, void *val, int size) {
    //put the values pointed to by val inside the physical memory at given virtual address
    //assume you can access val address directly by derefencing it
    //Also, it has the capability to put values inside the tlb

    va = unoffset(va);
    if(va==-1){
      printf("!!! Virtual address is bad. Returning and not freeing.\n");
      return;
    }
    //printf("writing 4 bytes to virtual address 0x%X\n",((unsigned int)va));

    char* val_ptr=(char*)val;
    // Translate VA to PA by checking TLB, if it misses, the searchTLB function calls translate()
    // pte_t* pa=searchTLB(va);
    //Check that all memory regions in the range are allocated
    int allocated=checkAllocated(va,size);
    if(allocated!=0){
      printf("!!! Not valid memory addresses. Returning\n");
      return;
    }

    char* pa=(char*)searchTLB(va);
    // printf("physical address 0x%X virtual addr: 0x%X\n",((unsigned int)pa-(unsigned int)physical_mem),va);
    if(pa==NULL){
      printf("!!! Not valid memory address. Returning\n");
      return;
    }

    // For i: 0 to Size
    int i=0;
    pthread_mutex_lock(&lock);
    for(i=0;i<size;i++){
      *(pa+i)=*(val_ptr+i);
    }
    pthread_mutex_unlock(&lock);


}

void printTLB(){
  int i = 0;
  for(i=0; i<TLB_SIZE; i++){
    printf("\ntlb_store->virtual_tags[%d] = 0x%x\n", i, tlb_store->virtual_tags[i]);
    printf("tlb_store->translations[%d] = 0x%x\n", i, tlb_store->translations[i]);
  }
}

void get_value(void *va, void *val, int size) {
    //put the values pointed to by va inside the physical memory at given val address
    //assume you can access val address directly by derefencing them
    //always check first the presence of translation inside the tlb before proceeding forward
    //for testing purpose:

    //reset va to unoffsetted value
    va = unoffset(va);
    if(va==-1){
      printf("!!! Virtual address is bad. Returning and not freeing.\n");
      return;
    }
    char* val_ptr=(char*)val;
    //Check that all memory regions in the range are allocated
    int allocated=checkAllocated(va,size);
    if(allocated!=0){
      printf("!!! Not valid memory addresses. Returning\n");
      return;
    }
    // Translate VA to PA by checking TLB, if it misses, the searchTLB function calls translate()
    pte_t* pa=searchTLB(va);
    if(pa==NULL){
      printf("!!! Not valid memory address. Returning\n");
      return;
    }


    pthread_mutex_lock(&lock);
    char* pa_ptr=(char*)pa;
    // For i: 0 to Size
    int i=0;
    for(i=0;i<size;i++){
      *(val_ptr+i)=*(pa_ptr+i);
    }
    pthread_mutex_unlock(&lock);

}

pte_t* searchTLB(pte_t* va){


  unsigned int tlb_index=getTLBIndex(va);
  unsigned int offset=getPageOffset(va);
  if(tlb_store->virtual_tags[tlb_index]==(unsigned int)va-offset){
    //TLB HIT
    pte_t* physical_address=tlb_store->translations[tlb_index];
    tlb_store->hits+=1;

    //delete page_num, its just for debugging
    int page_num=((int)physical_address-(int)physical_mem)/PGSIZE;

    //printf("TLB HIT! Physical address 0x%X     page number: %d\n",physical_address,page_num);

    return (char*)physical_address+offset;

  }
  else{
    //TLB MISS
    tlb_store->virtual_tags[tlb_index]=(unsigned int)va-offset;
    pte_t* physical_address=translate(page_dir,(unsigned int)(va)-offset);
    tlb_store->translations[tlb_index]=physical_address;

    //delete page_num, its just for debugging
    int page_num=((int)physical_address-(int)physical_mem)/PGSIZE;

    //printf("TLB MISS! Physical Address 0x%X    page number:%d\n",physical_address,page_num);
    tlb_store->misses+=1;
    // printf("offset: %d\n",offset);
     //printf("physical_address without offset: 0x%X\n",physical_address);

    return (unsigned int)physical_address+offset;

  }
}

void mat_mult(void *mat1, void *mat2, int size, void *answer) {
    //given two arrays of length: size * size
    //multiply them as matrices and store the computed result in answer
    //Hint: You will do indexing as [i * size + j] where i, j are the indices of matrix being accessed
    int *mat1_ptr = (int*)mat1;
    int *mat2_ptr = (int*)mat2;
    int *answer_ptr = (int*)answer;
    int ans=0;
    int i=0,j=0,k=0;
    printf("multiplying matrices!\n");
    for(i=0;i<size;i++){
      for(j=0;j<size;j++){
        for(k=0;k<size;k++){
          int mat1_val=0,mat2_val=0, old_answer_val=0;
          get_value(mat1+(i*size+k)*sizeof(int),&mat1_val,sizeof(int));
          get_value(mat2+(k*size+j)*sizeof(int),&mat2_val,sizeof(int));
          //printf("adding %d*%d\n",mat1_val,mat2_val);
          ans+=mat1_val*mat2_val;
        }
        put_value(answer+(i*size+j)*sizeof(int), &ans,sizeof(int));
        int a=0;
        get_value(answer+(i*size+j)*sizeof(int), &a,sizeof(int));

        ans=0;
      }

    }
}
