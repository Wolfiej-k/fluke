// programs/memthrash.c
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define ARRAY_SIZE (64 * 1024)

#define TOTAL_STEPS (20 * 1024 * 1024)

static uint32_t rng_state = 55555555u;

static int rand_int(void) {
  uint32_t x = rng_state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  rng_state = x;
  return (int)(x & 0x7fffffff);
}

// Fisher-Yates shuffle to create a single giant cycle.
static void create_random_cycle(int *arr, int n) {
  for (int i = 0; i < n; i++) {
    arr[i] = i;
  }

  for (int i = n - 1; i > 0; i--) {
    int j = rand_int() % (i + 1);
    int temp = arr[i];
    arr[i] = arr[j];
    arr[j] = temp;
  }

  // fix self-loops to ensure continuous chasing
  for (int i = 0; i < n; i++) {
    if (arr[i] == i) {
      int swap_idx = (i + 1) % n;
      int temp = arr[i];
      arr[i] = arr[swap_idx];
      arr[swap_idx] = temp;
    }
  }
}

int entry(void) {
  // Allocate a large memory block (32MB)
  int *memory = (int *)malloc(ARRAY_SIZE * sizeof(int));
  if (!memory) {
    perror("malloc");
    return 1;
  }

  // Setup the random pointer chain
  create_random_cycle(memory, ARRAY_SIZE);

  int current_idx = 0;
  int checksum = 0;

  // Pointer chasing
  for (int i = 0; i < TOTAL_STEPS; i++) {

    current_idx = memory[current_idx];

    if (current_idx >= ARRAY_SIZE || current_idx < 0) {
      current_idx = 0;
    }

    // Prevent dead code elim
    if ((i & 0xFF) == 0) {
      checksum ^= current_idx;
    }
  }

  printf("memthrash: steps=%d checksum=%x\n", TOTAL_STEPS, checksum);

  free(memory);
  return 0;
}

int main(void) { return entry(); }
