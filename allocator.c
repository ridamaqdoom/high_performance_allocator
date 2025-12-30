#define _GNU_SOURCE
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "allocator.h"

#define NUM_CLASSES 9
#define SLAB_SIZE (64 * 1024)

static size_t size_classes[NUM_CLASSES] = {
    16, 32, 64, 128, 256, 512, 1024, 2048, 4096
};

typedef struct block_header {
    struct block_header* next;
} block_header_t;

typedef struct slab {
    void* memory;
    block_header_t* free_list;
    size_t block_size;
    size_t total_blocks;
    struct slab* next;
} slab_t;

static slab_t* global_slabs[NUM_CLASSES] = {0};
static pthread_mutex_t global_lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct thread_cache {
    slab_t* slabs[NUM_CLASSES];
} thread_cache_t;

static __thread thread_cache_t cache = {0};

static int get_size_class(size_t size) {
    for (int i = 0; i < NUM_CLASSES; i++) {
        if (size <= size_classes[i])
            return i;
    }
    return -1;
}

static slab_t* slab_create(size_t block_size) {
    slab_t* slab = mmap(NULL, sizeof(slab_t),
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (slab == MAP_FAILED) return NULL;

    slab->memory = mmap(NULL, SLAB_SIZE,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (slab->memory == MAP_FAILED) return NULL;

    slab->block_size = block_size;
    slab->total_blocks = SLAB_SIZE / (sizeof(block_header_t) + block_size);
    slab->free_list = NULL;
    slab->next = NULL;

    char* ptr = (char*)slab->memory;
    for (size_t i = 0; i < slab->total_blocks; i++) {
        block_header_t* block = (block_header_t*)ptr;
        block->next = slab->free_list;
        slab->free_list = block;
        ptr += sizeof(block_header_t) + block_size;
    }

    return slab;
}

void* my_malloc(size_t size) {
    if (size == 0) return NULL;

    int class = get_size_class(size);

    if (class == -1) {
        size_t total = sizeof(size_t) + size;
        size_t* mem = mmap(NULL, total,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (mem == MAP_FAILED) return NULL;
        *mem = size;
        return (void*)(mem + 1);
    }

    slab_t* slab = cache.slabs[class];

    /* Fast path: thread-local slab */
    if (slab && slab->free_list) {
        block_header_t* block = slab->free_list;
        slab->free_list = block->next;
        return (void*)(block + 1);
    }

    /* Slow path: refill from global */
    pthread_mutex_lock(&global_lock);

    slab = global_slabs[class];
    if (!slab || !slab->free_list) {
        slab = slab_create(size_classes[class]);
        slab->next = global_slabs[class];
        global_slabs[class] = slab;
    }

    pthread_mutex_unlock(&global_lock);

    cache.slabs[class] = slab;

    block_header_t* block = slab->free_list;
    slab->free_list = block->next;

    return (void*)(block + 1);
}

void my_free(void* ptr) {
    if (!ptr) return;

    block_header_t* block = ((block_header_t*)ptr) - 1;
    size_t* possible_size = (size_t*)block;
    size_t size = *possible_size;

    int class = get_size_class(size);

    if (class == -1) {
        munmap(possible_size, sizeof(size_t) + size);
        return;
    }

    /* Return to thread-local slab */
    slab_t* slab = cache.slabs[class];
    if (slab) {
        block->next = slab->free_list;
        slab->free_list = block;
        return;
    }

    pthread_mutex_lock(&global_lock);
    slab = global_slabs[class];
    block->next = slab->free_list;
    slab->free_list = block;
    pthread_mutex_unlock(&global_lock);
}

