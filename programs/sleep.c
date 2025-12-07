#include <stdio.h>
#include <unistd.h>

void entry() {
    sleep(1);
    printf("hello!\n");
}

int main() {
    entry();
}
