#include "../my_vm.h"

int main() {

    printf("Allocating Three arrays of 400 bytes\n");
    void *a = a_malloc(4*100);
    int old_a = (int)a;
    void *b = a_malloc(4*100);
    void *c = a_malloc(4*100);
    int x = 1;
    int y, z;
    printf("Addresses of the Allocations: 0x%x, 0x%x, 0x%x\n", (int)a, (int)b, (int)c);

    printf("Storing some integers in the array to make a 5x5 matrix\n");
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            int address_a = (unsigned int)a + ((i * 5 * sizeof(int))) + (j * sizeof(int));
            int address_b = (unsigned int)b + ((i * 5 * sizeof(int))) + (j * sizeof(int));
            put_value((void *)address_a, &x, sizeof(int));
            put_value((void *)address_b, &x, sizeof(int));
        }
    }

    printf("Storing this 5x5 matrix in the arrays\n");
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            int address_a = (unsigned int)a + ((i * 5 * sizeof(int))) + (j * sizeof(int));
            int address_b = (unsigned int)b + ((i * 5 * sizeof(int))) + (j * sizeof(int));
            get_value((void *)address_a, &y, sizeof(int));
            get_value( (void *)address_b, &z, sizeof(int));
            printf("%d ", y);
        }
        printf("\n");
    }

    printf("Performing the matrix multiplication with itself!\n");
    mat_mult(a, b, 5, c);

    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            int address_c = (unsigned int)c + ((i * 5 * sizeof(int))) + (j * sizeof(int));
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

}
