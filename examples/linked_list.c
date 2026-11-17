// linked_list.c — lista encadeada com malloc/free: inserção ordenada,
// remoção, inversão e liberação de tudo, conferindo o heap no fim.
//
// expect-out: inserido: 1 3 4 5 7 8 9
// expect-out: sem pares: 1 3 5 7 9
// expect-out: invertida: 9 7 5 3 1
// expect-out: tamanho 5, soma 25, heap ok
#include <stdio.h>
#include <stdlib.h>

typedef struct Node Node;
struct Node {
  int value;
  Node *next;
};

// Insere mantendo a lista em ordem crescente.
static Node *insert_sorted(Node *head, int value) {
  Node *n = malloc(sizeof(Node));
  if (!n) {
    fprintf(stderr, "sem memoria\n");
    exit(1);
  }
  n->value = value;

  Node **link = &head;
  while (*link && (*link)->value < value)
    link = &(*link)->next;
  n->next = *link;
  *link = n;
  return head;
}

// Remove os nós para os quais keep() devolve 0.
static Node *filter(Node *head, int (*keep)(int)) {
  Node **link = &head;
  while (*link) {
    Node *n = *link;
    if (keep(n->value)) {
      link = &n->next;
    } else {
      *link = n->next;
      free(n);
    }
  }
  return head;
}

static Node *reverse(Node *head) {
  Node *prev = NULL;
  while (head) {
    Node *next = head->next;
    head->next = prev;
    prev = head;
    head = next;
  }
  return prev;
}

static void print(const char *label, Node *head) {
  printf("%s:", label);
  for (; head; head = head->next)
    printf(" %d", head->value);
  printf("\n");
}

static int is_odd(int v) { return v % 2 != 0; }

int main(void) {
  int values[] = {5, 3, 8, 1, 9, 4, 7};
  Node *list = NULL;

  for (int i = 0; i < 7; i++)
    list = insert_sorted(list, values[i]);
  print("inserido", list);

  list = filter(list, is_odd);
  print("sem pares", list);

  list = reverse(list);
  print("invertida", list);

  int count = 0, sum = 0;
  while (list) {
    Node *next = list->next;
    count++;
    sum += list->value;
    free(list);
    list = next;
  }
  printf("tamanho %d, soma %d, heap %s\n", count, sum,
         ks_heap_check() == 0 ? "ok" : "corrompido");
  return 0;
}
