#include "../my_vm.h"

int main() {

    printf("Allocating Three arrays of 400 bytes\n");
    void *a = a_malloc(4*20000);
    int old_a = (int)a;
    void *b = a_malloc(4*20000);
    void *c = a_malloc(4*20000);
    int x = 1;
    int y, z;
    //put_value((void *)(c+4097), &x, sizeof(int));
    //return;
    printf("Addresses of the Allocations: 0x%x, 0x%x, 0x%x\n", (int)a, (int)b, (int)c);
    int mat_size=50;
    printf("Storing some integers in the array to make a 5x5 matrix\n");
    for (int i = 0; i < mat_size; i++) {
        for (int j = 0; j < mat_size; j++) {
            int address_a = (unsigned int)a + ((i * mat_size * sizeof(int))) + (j * sizeof(int));
            int address_b = (unsigned int)b + ((i * mat_size * sizeof(int))) + (j * sizeof(int));
            put_value((void *)address_a, &x, sizeof(int));
            put_value((void *)address_b, &x, sizeof(int));
        }
    }

    printf("Storing this 5x5 matrix in the arrays\n");
    for (int i = 0; i < mat_size; i++) {
        for (int j = 0; j < mat_size; j++) {
            int address_a = (unsigned int)a + ((i * mat_size * sizeof(int))) + (j * sizeof(int));
            int address_b = (unsigned int)b + ((i * mat_size * sizeof(int))) + (j * sizeof(int));
            get_value((void *)address_a, &y, sizeof(int));
            get_value( (void *)address_b, &z, sizeof(int));
            printf("%d ", y);
        }
        printf("\n");
    }

    printf("Performing the matrix multiplication with itself!\n");
    mat_mult(a, b, mat_size, c);

    for (int i = 0; i < mat_size; i++) {
        for (int j = 0; j < mat_size; j++) {
            int address_c = (unsigned int)c + ((i * mat_size * sizeof(int))) + (j * sizeof(int));
            get_value((void *)address_c, &y, sizeof(int));
            printf("%d ", y);
        }
        printf("\n");
    }
    printf("Freeing the allocations!\n");
    a_free(a, 4*100);
    a_free(b, 4*100);
    a_free(c, 4*100);

    printf("Checking if the allocation was freed properly!\n");
    a = a_malloc(100*4);
    if ((int)a == old_a)
        printf("The allocation free works\n");
    else
        printf("The allocation free does not work\n");
    a_free(a, 4*100);

    /*void *addr = a_malloc(1024*1024*1024*2.0); // allocating 2 gigs
    a_free(addr+4096*2, 4096*3);
    a_free(addr+4096*10, 4096*5);

    void *more_addr1 = a_malloc(4096*2);
    void *more_addr2 = a_malloc(4096*2);
    void *more_addr3 = a_malloc(4096*2);
    void *more_addr4 = a_malloc(4096*1);
    void *more_addr5 = a_malloc(4096*1);
    a_free(more_addr5, 50000);
    void *more_addr6 = a_malloc(4096*13);*/
    printf("TLB HIT RATE: %.4f\n",tlb_store->hits/(tlb_store->hits+tlb_store->misses));
    printf("TLB MISS RATE: %.4f\n",tlb_store->misses/(tlb_store->hits+tlb_store->misses));

}
