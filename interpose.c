#include <stdlib.h>
#include <string.h>
#include "allocator.h"

/* malloc */
void* malloc(size_t size) {
    return my_malloc(size);
}


void free(void* ptr) {
    my_free(ptr);
}


void* calloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    void* ptr = my_malloc(total);
    if (ptr)
        memset(ptr, 0, total);
    return ptr;
}


void* realloc(void* ptr, size_t size) {
    if (!ptr)
        return my_malloc(size);

    if (size == 0) {
        my_free(ptr);
        return NULL;
    }

    void* new_ptr = my_malloc(size);
    if (!new_ptr)
        return NULL;

    memcpy(new_ptr, ptr, size); 
    my_free(ptr);
    return new_ptr;
}

