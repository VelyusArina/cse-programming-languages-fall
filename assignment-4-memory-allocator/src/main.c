#include "test.h"

int main() {
    test_malloc_success();
    test_free_one_block();
    test_free_two_block();
    memory_expansion();
    memory_not_expansion();
}

