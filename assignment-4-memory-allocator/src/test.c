#include "mem.h"
#include "mem_internals.h"
#include <stdio.h>

#include "test.h"

#define heap_start_const 4096

void debug (const char *fmt, ...);

static void test_start_print(char *number){
    printf("Starting test : %s\\n", number);
}

static void print_error_heap(char *number){
    fprintf(stderr, "Test failed : %s  error heap\\n", number);
}

static void print_error_malloc(char *number){
    fprintf(stderr, "Test failed : %s error malloc\\n", number);
}

static void print_success(char *number){
    fprintf(stdout, "Test ended successfully: %s\\n", number);
}


void test_malloc_success(){
    test_start_print("1");
    void *heap = heap_init(REGION_MIN_SIZE);
    if(!heap){
        print_error_heap("1");
        return;
    }
    void *malloc = _malloc(REGION_MIN_SIZE);

    if (!malloc){
        print_error_malloc("1");
        return;
    }
    _free(malloc);
    munmap(HEAP_START, heap_start_const);
    print_success("1");
}

void test_free_one_block(){
    test_start_print("2");
    void *heap = heap_init(REGION_MIN_SIZE);
    if(!heap){
        print_error_heap("2");
        return;
    }
    void *malloc1 = _malloc(REGION_MIN_SIZE);
    void *malloc2 = _malloc(REGION_MIN_SIZE);
    void *malloc3 = _malloc(REGION_MIN_SIZE);
    if(!malloc1 || !malloc2 || !malloc3){
        print_error_malloc("2");
        return;
    }
    debug_heap(stderr, heap);
    _free(malloc3);
    debug_heap(stderr, heap);
    _free(malloc2);
    _free(malloc1);
    munmap(HEAP_START, heap_start_const);
    print_success("2");
}

void test_free_two_block(){
    test_start_print("3");
    void *heap = heap_init(REGION_MIN_SIZE);
    if(!heap){
        print_error_heap("3");
        return;
    }
    void *malloc1 = _malloc(REGION_MIN_SIZE);
    void *malloc2 = _malloc(REGION_MIN_SIZE);
    void *malloc3 = _malloc(REGION_MIN_SIZE);
    void *malloc4 = _malloc(REGION_MIN_SIZE);
    void *malloc5 = _malloc(REGION_MIN_SIZE);
    if(!malloc1 || !malloc2 || !malloc3 || !malloc4 || !malloc5){
        print_error_malloc("3");
        return;
    }
    debug_heap(stderr, heap);
    _free(malloc1);
    _free(malloc2);
    debug_heap(stderr, heap);
    _free(malloc3);
    _free(malloc4);
    _free(malloc5);
    munmap(HEAP_START, heap_start_const);
    print_success("3");
}

void memory_expansion(){
    test_start_print("4");
    void *heap = heap_init(REGION_MIN_SIZE);
    if(!heap){
        print_error_heap("4");
        return;
    }
    void *malloc = _malloc((REGION_MIN_SIZE + 20));
    if(!malloc){
        print_error_malloc("4");
        return;
    }
    debug_heap(stderr, heap);
    _free(malloc);
    munmap(HEAP_START, heap_start_const * 4);
    print_success("4");
}
void memory_not_expansion(){
    test_start_print("5");
    void *heap = heap_init(REGION_MIN_SIZE);
    if(!heap){
        print_error_heap("5");
        return;
    }
    struct block_header *bh = (struct block_header *) heap;
    void *malloc1 = _malloc(bh->capacity.bytes);
    debug_heap(stderr, heap);
    void *map = mmap(bh->contents + bh->capacity.bytes, REGION_MIN_SIZE*2, PROT_READ | PROT_WRITE, MAP_PRIVATE, -1, 0);
    void *malloc2 = _malloc(REGION_MIN_SIZE);
    debug_heap(stderr, heap);
    _free(malloc1);
    _free(malloc2);
    munmap(map, size_from_capacity((block_capacity) {.bytes=5000}).bytes);
    munmap(heap, size_from_capacity((block_capacity) {.bytes=REGION_MIN_SIZE}).bytes);
    print_success("5");

}
