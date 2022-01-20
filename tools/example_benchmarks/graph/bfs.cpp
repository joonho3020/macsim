#include <string.h>
#include <stdio.h>

#include <vector>
#include <queue>

using namespace std;

#define MAX_NODES 100000
#define MAX_FEATURE 100

int V, E, F;
vector<int> adj[MAX_NODES];
int feature[MAX_NODES][MAX_FEATURE];
int visit[MAX_NODES];

queue<int> que;

extern "C" {
  int ROI_START(int x) {return 1;}
  int ROI_END(int x) {return 1;}

  int SIM_BEGIN(int x) {return 1;}
  int SIM_END(int x) {return 1;}
}

void run_other() {
  printf("hello world\n");
}

void run_bfs() {
  ROI_START(1);
  for (int i = 0; i < V; i++) {
    int cur_node = i + 1;
    if (visit[cur_node]) continue;

    que.push(cur_node);
    visit[cur_node] = 1;

    while (!que.empty()) {
      int u = que.front();
      que.pop();

      for (auto v : adj[u]) {
        if (visit[v]) continue;

        que.push(v);
        visit[v] = 1;

        for (int j = 0; j < F; j++) {
          feature[u][j] += feature[v][j];
        }
      }
    }
  }
  ROI_END(1);
}

void print_graph() {
  for (int i = 0; i < V; i++) {
    for (int j = 0; j < F; j++) {
      printf("%d ", feature[i+1][j]);
    }
    printf("\n");
  }
}

void get_input_graph() {
  scanf("%d %d %d", &V, &E, &F);

  // Nodes 1 ~ V
  for (int i = 0; i < V; i++) {
    int f;
    for (int j = 0; j < F; j++) {
      scanf("%d", &f);
      feature[i+1][j] = f;
    }
  }
  
  for (int i = 0; i < E; i++) {
    int u, v;
    scanf("%d %d", &u, &v);
    adj[u].push_back(v);
    adj[v].push_back(u);
  }

  memset(visit, 0, MAX_NODES * sizeof(int));
}

int main (int argc, char** argv) {
  get_input_graph();

  SIM_BEGIN(1);
  run_other();
  run_bfs();
  SIM_END(1);

/* print_graph(); */
  return 0;
}
