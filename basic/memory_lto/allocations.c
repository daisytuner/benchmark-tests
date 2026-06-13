#include <stdlib.h>

__attribute__((noinline))
void malloc_double(void** ptr)
{
    *ptr = malloc(16384 * sizeof(double));
}

__attribute__((noinline))
void free_double(void* ptr)
{
    free(ptr);
}

int main() {

    double* ptr1 = (double*) malloc(16384 * sizeof(double));

    void* ptr2;
    malloc_double(&ptr2);

    free_double(ptr1);
    free(ptr2);

    return 0;
}
