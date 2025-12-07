#include <stdio.h>
#include <stdlib.h>

void entry() {
    void* addr = malloc(16);
    printf("malloc: %p, return: %p\n", malloc, addr);
    free(addr);
}

int main() {
    entry();
}
