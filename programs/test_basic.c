#include <stdio.h>

__attribute__((noinline))
void entry(void) {
    printf("test_basic: hello from basic test\n");
    printf("test_basic: simple dynamically loaded library\n");
    printf("test_basic: PASS\n");
}

int main() {
    entry();
}
