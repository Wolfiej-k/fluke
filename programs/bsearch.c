// programs/bsearch.c
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static uint32_t rng_state_bs = 13579u;

static int rand_int_bs(void) {
  uint32_t x = rng_state_bs;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  rng_state_bs = x;
  return (int)(x & 0x7fffffff);
}

static int cmp_int(const void *a, const void *b) {
  int ia = *(const int *)a;
  int ib = *(const int *)b;
  return (ia > ib) - (ia < ib);
}

static int binsearch(const int *arr, int n, int key) {
  int lo = 0, hi = n - 1;
  while (lo <= hi) {
    int mid = lo + (hi - lo) / 2;
    int v = arr[mid];
    if (v == key)
      return mid;
    if (v < key)
      lo = mid + 1;
    else
      hi = mid - 1;
  }
  return -1;
}

int entry(void) {
  const int N = 300000;
  int *arr = (int *)malloc(N * sizeof(int));
  if (!arr) {
    perror("malloc");
    return 1;
  }

  for (int i = 0; i < N; i++) {
    arr[i] = rand_int_bs();
  }

  qsort(arr, N, sizeof(int), cmp_int);

  const int Q = 100000;
  int hits = 0;

  // half queries guaranteed hit, half likely miss
  for (int q = 0; q < Q; q++) {
    int key;
    if (q % 2 == 0) {
      key = arr[(q * 9973) % N]; // some existing element
    } else {
      key = rand_int_bs() ^ 0xabcdef;
    }
    int idx = binsearch(arr, N, key);
    if (idx >= 0)
      hits++;
  }

  printf("bsearch: N=%d Q=%d hits=%d\n", N, Q, hits);

  free(arr);
  return 0;
}

int main(void) { return entry(); }
