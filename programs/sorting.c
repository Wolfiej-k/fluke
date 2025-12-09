// programs/sorting.c
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static uint32_t rng_state_sort = 987654321u;

static int rand_int_sort(void) {
  uint32_t x = rng_state_sort;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  rng_state_sort = x;
  return (int)(x & 0x7fffffff);
}

static void merge(int *arr, int *tmp, int left, int mid, int right) {
  int i = left;
  int j = mid + 1;
  int k = left;

  while (i <= mid && j <= right) {
    if (arr[i] <= arr[j])
      tmp[k++] = arr[i++];
    else
      tmp[k++] = arr[j++];
  }
  while (i <= mid)
    tmp[k++] = arr[i++];
  while (j <= right)
    tmp[k++] = arr[j++];

  for (i = left; i <= right; i++)
    arr[i] = tmp[i];
}

static void mergesort_rec(int *arr, int *tmp, int left, int right) {
  if (left >= right)
    return;
  int mid = left + (right - left) / 2;
  mergesort_rec(arr, tmp, left, mid);
  mergesort_rec(arr, tmp, mid + 1, right);
  merge(arr, tmp, left, mid, right);
}

static void mergesort(int *arr, int n) {
  int *tmp = (int *)malloc(n * sizeof(int));
  if (!tmp) {
    perror("malloc");
    exit(1);
  }
  mergesort_rec(arr, tmp, 0, n - 1);
  free(tmp);
}

int entry(void) {
  const int N = 500000;
  int *arr = (int *)malloc(N * sizeof(int));
  if (!arr) {
    perror("malloc");
    return 1;
  }

  for (int i = 0; i < N; i++) {
    arr[i] = rand_int_sort();
  }

  mergesort(arr, N);

  // sanity check: verify non-decreasing
  int ok = 1;
  for (int i = 1; i < N; i++) {
    if (arr[i] < arr[i - 1]) {
      ok = 0;
      break;
    }
  }

  printf("sorting: N=%d sorted=%s\n", N, ok ? "yes" : "no");

  free(arr);
  return ok ? 0 : 1;
}

int main(void) { return entry(); }
