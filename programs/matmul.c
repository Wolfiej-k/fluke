// programs/matmul.c
#include <stdio.h>
#include <stdlib.h>

static inline double idx(double *M, int n, int i, int j) {
  return M[(size_t)i * (size_t)n + (size_t)j];
}

static inline double *idx_ref(double *M, int n, int i, int j) {
  return &M[(size_t)i * (size_t)n + (size_t)j];
}

int entry(void) {
  const int N = 512;
  size_t total = (size_t)N * (size_t)N;

  double *A = (double *)malloc(total * sizeof(double));
  double *B = (double *)malloc(total * sizeof(double));
  double *C = (double *)malloc(total * sizeof(double));

  if (!A || !B || !C) {
    perror("malloc");
    free(A);
    free(B);
    free(C);
    return 1;
  }

  // initialize A and B with deterministic values
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      *idx_ref(A, N, i, j) = (double)((i + 1) * (j + 1));
      *idx_ref(B, N, i, j) = (double)((i == j) ? 1.0 : 0.0);
      *idx_ref(C, N, i, j) = 0.0;
    }
  }

  for (int i = 0; i < N; i++) {
    for (int k = 0; k < N; k++) {
      double aik = idx(A, N, i, k);
      for (int j = 0; j < N; j++) {
        *idx_ref(C, N, i, j) += aik * idx(B, N, k, j);
      }
    }
  }

  // basic checksum to prevent dead-code elimination
  double checksum = 0.0;
  for (int i = 0; i < N; i++) {
    checksum += idx(C, N, i, i);
  }

  printf("matmul: N=%d checksum=%f\n", N, checksum);

  free(A);
  free(B);
  free(C);
  return 0;
}

int main(void) { return entry(); }
