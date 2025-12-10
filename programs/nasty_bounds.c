#include <stdio.h>

__attribute__((noinline))
void entry() {
    int good = *(int *)10000000000;
    int bad = *(int *)1000;
    printf("%d %d\n", good, bad);
}

int main() {
    entry();
    return 0;
}
