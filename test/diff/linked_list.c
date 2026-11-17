// Lista simplesmente encadeada com nós numa arena estática (sem malloc):
// inserção ordenada, remoção, inversão, merge e lista livre.
#include "kstest.h"
#include <stddef.h>

typedef struct Node {
  int key;
  short tag;
  struct Node *next;
} Node;

#define ARENA 128
static Node arena[ARENA];
static Node *free_list;

static void arena_init(void) {
  free_list = NULL;
  for (int i = ARENA - 1; i >= 0; i--) {
    arena[i].next = free_list;
    free_list = &arena[i];
  }
}

static Node *node_new(int key, short tag) {
  Node *n = free_list;
  if (!n)
    return NULL;
  free_list = n->next;
  n->key = key;
  n->tag = tag;
  n->next = NULL;
  return n;
}

static void node_free(Node *n) {
  n->next = free_list;
  free_list = n;
}

static void insert_sorted(Node **head, Node *n) {
  Node **pp = head;
  while (*pp && (*pp)->key < n->key)
    pp = &(*pp)->next;
  n->next = *pp;
  *pp = n;
}

static int remove_key(Node **head, int key) {
  int removed = 0;
  for (Node **pp = head; *pp;) {
    if ((*pp)->key == key) {
      Node *dead = *pp;
      *pp = dead->next;
      node_free(dead);
      removed++;
    } else {
      pp = &(*pp)->next;
    }
  }
  return removed;
}

static Node *reverse(Node *h) {
  Node *prev = NULL;
  while (h) {
    Node *nx = h->next;
    h->next = prev;
    prev = h;
    h = nx;
  }
  return prev;
}

static Node *merge(Node *a, Node *b) {
  Node dummy;
  Node *t = &dummy;
  dummy.next = NULL;
  while (a && b) {
    if (a->key <= b->key) { t->next = a; a = a->next; }
    else { t->next = b; b = b->next; }
    t = t->next;
  }
  t->next = a ? a : b;
  return dummy.next;
}

static int length(const Node *h) {
  int n = 0;
  for (; h; h = h->next)
    n++;
  return n;
}

static int free_count(void) { return length(free_list); }

static void print(const char *label, const Node *h) {
  ks_put_str(label);
  ks_put_str(":");
  for (; h; h = h->next) {
    ks_put_chr(' ');
    ks_put_int(h->key);
    if (h->tag) {
      ks_put_chr('#');
      ks_put_int(h->tag);
    }
  }
  ks_put_chr('\n');
}

int main(void) {
  arena_init();
  Node *a = NULL, *b = NULL;
  int keys[] = {42, -7, 13, 99, 0, 13, 5, -100, 64, 13, 8};
  for (unsigned i = 0; i < sizeof keys / sizeof keys[0]; i++)
    insert_sorted(&a, node_new(keys[i], (short)i));
  for (int i = 0; i < 10; i++)
    insert_sorted(&b, node_new(i * 11 - 30, 0));
  print("a", a);
  print("b", b);
  ks_put_kv("removed13", remove_key(&a, 13));
  ks_put_kv("removed1000", remove_key(&a, 1000));
  print("a", a);
  Node *m = merge(a, b);
  print("merged", m);
  ks_put_kv("len", length(m));
  m = reverse(m);
  print("reversed", m);
  ks_put_kv("free", free_count());
  // Esvazia e reaproveita a arena inteira.
  while (m) {
    Node *nx = m->next;
    node_free(m);
    m = nx;
  }
  ks_put_kv("free", free_count());
  Node *big = NULL;
  int made = 0;
  for (Node *n; (n = node_new((made * 37) % 101, 0)) != NULL; made++)
    insert_sorted(&big, n);
  ks_put_kv("made", made);
  long sum = 0;
  int prev = -1, ok = 1;
  for (Node *n = big; n; n = n->next) {
    sum += n->key;
    if (n->key < prev) ok = 0;
    prev = n->key;
  }
  ks_put_kv("sum", (int)sum);
  ks_put_kv("ordered", ok);
  ks_put_kv("sizeof(Node)", (int)sizeof(Node));
  ks_put_kv("offsetof(next)", (int)offsetof(Node, next));
  return 0;
}
