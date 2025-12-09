// programs/treap.c
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Node {
  int key;
  int priority;
  struct Node *left;
  struct Node *right;
} Node;

static uint32_t rng_state = 123456789u;

static int rand_int(void) {
  // Simple xorshift RNG, deterministic
  uint32_t x = rng_state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  rng_state = x;
  return (int)(x & 0x7fffffff);
}

static Node *new_node(int key) {
  Node *n = (Node *)malloc(sizeof(Node));
  if (!n) {
    perror("malloc");
    exit(1);
  }
  n->key = key;
  n->priority = rand_int();
  n->left = n->right = NULL;
  return n;
}

static Node *rotate_right(Node *y) {
  Node *x = y->left;
  Node *T2 = x->right;
  x->right = y;
  y->left = T2;
  return x;
}

static Node *rotate_left(Node *x) {
  Node *y = x->right;
  Node *T2 = y->left;
  y->left = x;
  x->right = T2;
  return y;
}

static Node *treap_insert(Node *root, int key) {
  if (root == NULL)
    return new_node(key);

  if (key < root->key) {
    root->left = treap_insert(root->left, key);
    if (root->left && root->left->priority > root->priority)
      root = rotate_right(root);
  } else if (key > root->key) {
    root->right = treap_insert(root->right, key);
    if (root->right && root->right->priority > root->priority)
      root = rotate_left(root);
  } else {
    // duplicate key, ignore
  }
  return root;
}

static int treap_search(Node *root, int key) {
  while (root != NULL) {
    if (key < root->key)
      root = root->left;
    else if (key > root->key)
      root = root->right;
    else
      return 1;
  }
  return 0;
}

static void treap_free(Node *root) {
  if (!root)
    return;
  treap_free(root->left);
  treap_free(root->right);
  free(root);
}

int entry(void) {
  const int N = 50000;
  int *keys = (int *)malloc(N * sizeof(int));
  if (!keys) {
    perror("malloc");
    return 1;
  }

  // deterministic "random-ish" keys
  for (int i = 0; i < N; i++) {
    keys[i] = rand_int();
  }

  Node *root = NULL;
  for (int i = 0; i < N; i++) {
    root = treap_insert(root, keys[i]);
  }

  // search for all keys
  int found_count = 0;
  for (int i = 0; i < N; i++) {
    found_count += treap_search(root, keys[i]);
  }

  // search for some missing keys
  for (int i = 0; i < N; i += 10) {
    (void)treap_search(root, keys[i] ^ 0x55555555);
  }

  printf("treap: inserted=%d found=%d\n", N, found_count);

  treap_free(root);
  free(keys);
  return 0;
}

int main(void) { return entry(); }
