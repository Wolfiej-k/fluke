#include <stdio.h>

void entry() {
    int val = *(int *)1000;
    printf("val: %d\n", val);
}

int main() {
    entry();
}
