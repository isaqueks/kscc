// Grafos: Dijkstra O(V^2) em matriz de adjacência, BFS em grade com fila
// circular, Floyd-Warshall, ordenação topológica (Kahn) e componentes por
// union-find com compressão de caminho.
#include "kstest.h"

#define V 9
#define INF 0x3FFFFFFF

static const int adj[V][V] = {
    {0, 4, 0, 0, 0, 0, 0, 8, 0},  {4, 0, 8, 0, 0, 0, 0, 11, 0}, {0, 8, 0, 7, 0, 4, 0, 0, 2},
    {0, 0, 7, 0, 9, 14, 0, 0, 0}, {0, 0, 0, 9, 0, 10, 0, 0, 0}, {0, 0, 4, 14, 10, 0, 2, 0, 0},
    {0, 0, 0, 0, 0, 2, 0, 1, 6},  {8, 11, 0, 0, 0, 0, 1, 0, 7}, {0, 0, 2, 0, 0, 0, 6, 7, 0},
};

static void dijkstra(int src, int dist[V], int prev[V]) {
  int done[V] = {0};
  for (int i = 0; i < V; i++) dist[i] = INF, prev[i] = -1;
  dist[src] = 0;
  for (int it = 0; it < V; it++) {
    int u = -1;
    for (int i = 0; i < V; i++)
      if (!done[i] && (u < 0 || dist[i] < dist[u])) u = i;
    done[u] = 1;
    for (int v = 0; v < V; v++)
      if (adj[u][v] && dist[u] + adj[u][v] < dist[v]) {
        dist[v] = dist[u] + adj[u][v];
        prev[v] = u;
      }
  }
}

static const char maze[7][12] = {
    "S..#.......", ".#.#.####..", ".#...#..#.#", ".####.#.#..", "...#..#...#", "#.#..##.#..",
    "....#....#E",
};

static int bfs(int *visited_out) {
  int q[77], head = 0, tail = 0;
  int dist[7][11];
  for (int r = 0; r < 7; r++)
    for (int c = 0; c < 11; c++) dist[r][c] = -1;
  dist[0][0] = 0;
  q[tail++] = 0;
  int visited = 0;
  static const int dr[4] = {1, -1, 0, 0}, dc[4] = {0, 0, 1, -1};
  while (head != tail) {
    int cur = q[head++ % 77];
    int r = cur / 11, c = cur % 11;
    visited++;
    if (maze[r][c] == 'E') { *visited_out = visited; return dist[r][c]; }
    for (int k = 0; k < 4; k++) {
      int nr = r + dr[k], nc = c + dc[k];
      if (nr < 0 || nr >= 7 || nc < 0 || nc >= 11) continue;
      if (maze[nr][nc] == '#' || dist[nr][nc] >= 0) continue;
      dist[nr][nc] = dist[r][c] + 1;
      q[tail++ % 77] = nr * 11 + nc;
    }
  }
  *visited_out = visited;
  return -1;
}

static int parent[16], rnk[16];
static int find(int x) { return parent[x] == x ? x : (parent[x] = find(parent[x])); }
static void unite(int a, int b) {
  a = find(a), b = find(b);
  if (a == b) return;
  if (rnk[a] < rnk[b]) { int t = a; a = b; b = t; }
  parent[b] = a;
  if (rnk[a] == rnk[b]) rnk[a]++;
}

int main(void) {
  int dist[V], prev[V];
  dijkstra(0, dist, prev);
  for (int i = 0; i < V; i++) {
    ks_put_int(i);
    ks_put_str(": ");
    ks_put_int(dist[i]);
    ks_put_str(" via");
    for (int p = i; p >= 0; p = prev[p]) {
      ks_put_chr(' ');
      ks_put_int(p);
    }
    ks_put_chr('\n');
  }
  int fw[V][V];
  for (int i = 0; i < V; i++)
    for (int j = 0; j < V; j++) fw[i][j] = i == j ? 0 : adj[i][j] ? adj[i][j] : INF;
  for (int k = 0; k < V; k++)
    for (int i = 0; i < V; i++)
      for (int j = 0; j < V; j++)
        if (fw[i][k] + fw[k][j] < fw[i][j]) fw[i][j] = fw[i][k] + fw[k][j];
  int same = 1, total = 0;
  for (int i = 0; i < V; i++) {
    same &= fw[0][i] == dist[i];
    for (int j = 0; j < V; j++) total += fw[i][j];
  }
  ks_put_kv("fw==dijkstra", same);
  ks_put_kv("fw total", total);
  int visited;
  int d = bfs(&visited);
  ks_put_kv("bfs dist", d);
  ks_put_kv("bfs visited", visited);
  // topológica: arestas i -> j com j = (i*3+1)%10 se j > i, e i -> i+2
  int indeg[10] = {0}, edges[10][10] = {{0}};
  for (int i = 0; i < 10; i++) {
    int j = (i * 3 + 1) % 10;
    if (j > i) edges[i][j] = 1;
    if (i + 2 < 10) edges[i][i + 2] = 1;
  }
  for (int i = 0; i < 10; i++)
    for (int j = 0; j < 10; j++) indeg[j] += edges[i][j];
  int order[10], no = 0, queue[10], qh = 0, qt = 0;
  for (int i = 0; i < 10; i++) if (!indeg[i]) queue[qt++] = i;
  while (qh < qt) {
    int u = queue[qh++];
    order[no++] = u;
    for (int v = 0; v < 10; v++)
      if (edges[u][v] && --indeg[v] == 0) queue[qt++] = v;
  }
  ks_put_str("topo:");
  for (int i = 0; i < no; i++) { ks_put_chr(' '); ks_put_int(order[i]); }
  ks_put_chr('\n');
  for (int i = 0; i < 16; i++) parent[i] = i, rnk[i] = 0;
  int pairs[][2] = {{0, 1}, {2, 3}, {1, 3}, {4, 5}, {6, 7}, {5, 7}, {8, 9}, {10, 11}, {9, 11}, {12, 12}, {3, 0}};
  for (unsigned i = 0; i < sizeof pairs / sizeof pairs[0]; i++) unite(pairs[i][0], pairs[i][1]);
  int comps = 0;
  for (int i = 0; i < 16; i++) comps += find(i) == i;
  ks_put_kv("components", comps);
  ks_put_kv("same(0,2)", find(0) == find(2));
  ks_put_kv("same(0,4)", find(0) == find(4));
  return 0;
}
