#include <stdio.h>
#include "allocator.h"

int main() {
    int* a = my_malloc(sizeof(int));
    *a = 42;

    printf("Value: %d\n", *a);

    my_free(a);
    return 0;
}

