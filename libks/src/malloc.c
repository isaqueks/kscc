// malloc, calloc, realloc e free da libks (doc 05 §3).
//
//   heap_lo ──► [cab][bloco][cab][bloco] … ──► heap_hi … stack_limit ◄── sp
//
// O heap vai do fim do .bss (MEM_INFO heap_start) até stack_limit - 64:
// a pilha cresce para baixo até stack_limit, e a margem dá folga para a
// VM acusar FAULT_STACK antes de a pilha pisar no heap. Na primeira
// chamada a região inteira vira um único bloco livre.
//
// Cada bloco tem um cabeçalho de 8 bytes: o tamanho total do bloco
// (cabeçalho incluso, múltiplo de 4) com o bit 0 marcando "livre", e o
// tamanho do bloco fisicamente anterior (0 no primeiro), que permite
// achar o vizinho de trás ao liberar. Blocos livres guardam, no começo da
// área útil, os elos de uma lista duplamente encadeada; por isso o menor
// bloco tem 16 bytes. A busca é first-fit na lista; o resto de um bloco
// grande demais volta à lista; free junta o bloco com os vizinhos livres,
// então nunca há dois blocos livres adjacentes.
//
// Com -DKS_MALLOC_DEBUG (make -C libks KS_MALLOC_DEBUG=1): blocos
// liberados são preenchidos com 0xDD, e malloc/free/realloc conferem os
// cabeçalhos (e o preenchimento, ao reaproveitar um bloco); corrupção ou
// free duplo imprime "malloc: ..." e chama abort().
#include <errno.h>
#include <ksvm/syscall.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  size_t size;       // tamanho total | FREE_BIT
  size_t prev_size;  // tamanho do bloco anterior; 0 no primeiro
} Header;

typedef struct FreeBlock FreeBlock;
struct FreeBlock {
  Header h;
  FreeBlock *next;
  FreeBlock *prev;
};

#define FREE_BIT 1u
#define HDR_SIZE ((size_t)sizeof(Header))
#define MIN_BLOCK ((size_t)sizeof(FreeBlock))
#define STACK_MARGIN 64
#define DEAD_BYTE 0xDD

static char *heap_lo;  // primeiro bloco; NULL antes de heap_init
static char *heap_hi;  // fim do heap (exclusivo)
static FreeBlock *free_list;

static size_t block_size(Header *h) { return h->size & ~FREE_BIT; }
static int is_free(Header *h) { return (h->size & FREE_BIT) != 0; }
static Header *header_of(void *p) { return (Header *)((char *)p - HDR_SIZE); }

static Header *next_block(Header *h) {
  char *n = (char *)h + block_size(h);
  return n < heap_hi ? (Header *)n : NULL;
}

static Header *prev_block(Header *h) {
  return h->prev_size ? (Header *)((char *)h - h->prev_size) : NULL;
}

// Grava tamanho e estado e mantém o prev_size do vizinho seguinte.
static void set_block(Header *h, size_t size, unsigned free_bit) {
  h->size = size | free_bit;
  Header *n = next_block(h);
  if (n)
    n->prev_size = size;
}

static void list_insert(FreeBlock *b) {
  b->prev = NULL;
  b->next = free_list;
  if (free_list)
    free_list->prev = b;
  free_list = b;
}

static void list_remove(FreeBlock *b) {
  if (b->prev)
    b->prev->next = b->next;
  else
    free_list = b->next;
  if (b->next)
    b->next->prev = b->prev;
}

#ifdef KS_MALLOC_DEBUG
static _Noreturn void corrupt(const char *what) {
  ks_sys_puts("malloc: ", 8);
  ks_sys_puts(what, strlen(what));
  ks_sys_puts("\n", 1);
  abort();
}

// Cabeçalho plausível e coerente com os dois vizinhos.
static void check_block(Header *h) {
  char *p = (char *)h;
  size_t size = block_size(h);

  if (p < heap_lo || p >= heap_hi || ((size_t)p & 3))
    corrupt("ponteiro fora do heap");
  if (size < MIN_BLOCK || (size & 3) || size > (size_t)(heap_hi - p))
    corrupt("cabecalho corrompido");
  Header *n = next_block(h);
  if (n && n->prev_size != size)
    corrupt("cabecalho do bloco seguinte corrompido");
  Header *pv = prev_block(h);
  if (pv && ((char *)pv < heap_lo || block_size(pv) != h->prev_size))
    corrupt("cabecalho do bloco anterior corrompido");
}

// Um bloco livre ainda cheio de 0xDD: ninguém escreveu nele depois do free.
static void check_dead(Header *h) {
  unsigned char *p = (unsigned char *)h + MIN_BLOCK;
  unsigned char *end = (unsigned char *)h + block_size(h);

  for (; p < end; p++)
    if (*p != DEAD_BYTE)
      corrupt("bloco livre modificado depois do free");
}
#else
#define check_block(h) ((void)0)
#define check_dead(h) ((void)0)
#endif

// Marca o bloco como livre, junta com os vizinhos livres e põe na lista.
static void release(Header *h) {
  size_t size = block_size(h);
  Header *n = next_block(h);
  Header *pv = prev_block(h);

  if (n && is_free(n)) {
    list_remove((FreeBlock *)n);
    size += block_size(n);
  }
  if (pv && is_free(pv)) {
    list_remove((FreeBlock *)pv);
    size += block_size(pv);
    h = pv;
  }
#ifdef KS_MALLOC_DEBUG
  memset((char *)h + MIN_BLOCK, DEAD_BYTE, size - MIN_BLOCK);
#endif
  set_block(h, size, FREE_BIT);
  list_insert((FreeBlock *)h);
}

// Reduz um bloco em uso para `need` bytes se a sobra formar um bloco.
static void shrink(Header *h, size_t need) {
  size_t size = block_size(h);

  if (size - need < MIN_BLOCK)
    return;
  Header *rest = (Header *)((char *)h + need);
  set_block(h, need, 0);  // também grava rest->prev_size
  rest->size = size - need;
  set_block(rest, size - need, 0);
  release(rest);
}

static int heap_init(void) {
  struct ks_mem_info mi;

  ks_sys_mem_info(&mi);
  heap_lo = (char *)((mi.heap_start + 3) & ~3u);
  heap_hi = heap_lo;
  if (mi.stack_limit > mi.heap_start + STACK_MARGIN + MIN_BLOCK)
    heap_hi = (char *)((mi.stack_limit - STACK_MARGIN) & ~3u);
  if (heap_hi - heap_lo < (int)MIN_BLOCK)
    return 0;

  Header *h = (Header *)heap_lo;
  h->prev_size = 0;
  h->size = heap_hi - heap_lo;
  release(h);
  return 1;
}

// Tamanho de bloco para `n` bytes úteis, ou 0 se não pode caber.
static size_t block_for(size_t n) {
  if (n > (size_t)(heap_hi - heap_lo))
    return 0;
  size_t need = (n + HDR_SIZE + 3) & ~(size_t)3;
  return need < MIN_BLOCK ? MIN_BLOCK : need;
}

// malloc(0) devolve um bloco mínimo (ponteiro único, que pode ir a free).
void *malloc(size_t n) {
  if (!heap_lo && !heap_init()) {
    errno = ENOMEM;
    return NULL;
  }

  size_t need = block_for(n);
  for (FreeBlock *b = free_list; need && b; b = b->next) {
    if (block_size(&b->h) < need)
      continue;
    check_block(&b->h);
    check_dead(&b->h);
    list_remove(b);
    set_block(&b->h, block_size(&b->h), 0);
    shrink(&b->h, need);
    return (char *)b + HDR_SIZE;
  }
  errno = ENOMEM;
  return NULL;
}

void free(void *p) {
  if (!p)
    return;
  Header *h = header_of(p);
  check_block(h);
#ifdef KS_MALLOC_DEBUG
  if (is_free(h))
    corrupt("free duplo");
#endif
  release(h);
}

void *calloc(size_t n, size_t size) {
  if (size && n > (size_t)-1 / size) {
    errno = ENOMEM;
    return NULL;
  }
  void *p = malloc(n * size);
  if (p)
    memset(p, 0, n * size);
  return p;
}

// Encolhe no lugar; cresce no lugar se o vizinho seguinte está livre e
// basta; senão aloca, copia e libera. Em falha o bloco original fica
// intacto (C11 7.22.3.5).
void *realloc(void *p, size_t n) {
  if (!p)
    return malloc(n);
  if (n == 0) {
    free(p);
    return NULL;
  }

  Header *h = header_of(p);
  check_block(h);
  size_t need = block_for(n);
  size_t have = block_size(h);
  if (!need) {
    errno = ENOMEM;
    return NULL;
  }
  if (need <= have) {
    shrink(h, need);
    return p;
  }

  Header *nx = next_block(h);
  if (nx && is_free(nx) && have + block_size(nx) >= need) {
    list_remove((FreeBlock *)nx);
    set_block(h, have + block_size(nx), 0);
    shrink(h, need);
    return p;
  }

  void *q = malloc(n);
  if (!q)
    return NULL;
  memcpy(q, p, have - HDR_SIZE);
  free(p);
  return q;
}

// Percorre o heap inteiro: tamanhos e prev_size coerentes, blocos
// cobrindo exatamente [heap_lo, heap_hi), nenhum par de livres adjacentes
// e a lista de livres com exatamente os blocos marcados livres.
int ks_heap_check(void) {
  if (!heap_lo)
    return 0;

  int nfree = 0;
  size_t prev = 0;
  int prev_free = 0;
  char *p = heap_lo;
  while (p < heap_hi) {
    Header *h = (Header *)p;
    size_t size = block_size(h);
    if (size < MIN_BLOCK || (size & 3) || size > (size_t)(heap_hi - p))
      return -1;
    if (h->prev_size != prev || (prev_free && is_free(h)))
      return -1;
    nfree += is_free(h);
    prev = size;
    prev_free = is_free(h);
    p += size;
  }
  if (p != heap_hi)
    return -1;

  for (FreeBlock *b = free_list; b; b = b->next) {
    if (!is_free(&b->h) || (char *)b < heap_lo || (char *)b >= heap_hi)
      return -1;
    nfree--;
  }
  return nfree == 0 ? 0 : -1;
}
