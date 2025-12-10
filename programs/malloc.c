#include <stdio.h>
#include <stdlib.h>

__attribute__((noinline)) void entry() {
    void* addr = malloc(16);
    printf("malloc: %p, return: %p\n", malloc, addr);
    free(addr);
}

int main() {
    entry();
}
