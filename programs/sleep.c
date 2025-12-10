#include <stdio.h>
#include <unistd.h>

__attribute__((noinline)) void entry() {
    sleep(1);
    printf("hello!\n");
}

int main() {
    entry();
}
