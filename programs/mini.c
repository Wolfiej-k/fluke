#include <stdio.h>

int x = 42;
int y = 67;

__attribute__((visibility("default")))
__attribute__((noinline)) void entry() {
    int bad = 0;
    if (x + y == 109) {
        x = 67;
        y = 42;
    } else {
        bad = 1;
    }
    printf("bad: %d\n", bad);
}

int main() {
    entry();
}
