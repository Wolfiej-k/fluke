#include <stdio.h>

__attribute__((noinline)) void entry() {
    printf("Hello, World!\n");
}

int main() {
    entry();
}
