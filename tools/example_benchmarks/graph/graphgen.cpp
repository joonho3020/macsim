#include <iostream>
#include <string>
#include <fstream>
#include <set>
#include <random>

using namespace std;
typedef pair<int, int> pii;

#define MAX_DEG 10
int adj_count[MAX_DEG + 1];
float alpha = 0.5;

int V, E, F;

set<pii> sampled;

float power(float x, int y) {
  float ret = 1;
  for (int i = 0; i < y; i++)
    ret *= x;
  return ret;
}

int main(int argc, char** argv) {
  if (argc != 5) {
    std::cout << "usage : ./graphgen V E F outfile" << std::endl;
  }

  V = atoi(argv[1]);
  E = atoi(argv[2]);
  F = atoi(argv[3]);
  string outfile = argv[4];

  ofstream file;
  file.open(outfile);

  file << V << " " << E << " " << F << endl;
  for (int i = 0; i < V; i++) {
    for (int j = 0; j < F; j++) {
      file << i << " ";
    }
    file << endl;
  }

  double avg_deg = max((double)(E / V), 1.0);

  default_random_engine generator;
  exponential_distribution<double> exp_dist(avg_deg);
  uniform_int_distribution<int> uni_dist(1, V);

  for (int cnt = 0, v = 1; cnt < E && v <= V; v++) {
    // sample degree
    int deg = (int)exp_dist(generator);
    deg = max(deg, 1);

    // edge overflow
    if (cnt + deg > E || (cnt < E && v == V)) deg = E - cnt;

/* cout << deg << endl; */
    
    // update total number of edges
    cnt += deg;

    // sample deg adjacent edges for node "v"
    while (deg) {
      // sample adjacent node
      int u = uni_dist(generator);

      // skip itself
      if (u == v) continue;

      // already sampled
      // allow overlapping edges for now
/* pii edge; */
/* if (u < v) { */
/* edge.first = u; */
/* edge.second = v; */
/* } else { */
/* edge.first = v; */
/* edge.second = u; */
/* } */
/* if (sampled.find(edge) != sampled.end()) continue; */

/* sampled.insert(edge); */
      file << v << " " << u << endl;
      deg--;
    }
  }
}
