#include <string.h>
#include <stdio.h>

#include <vector>
#include <queue>

using namespace std;

extern "C" {
  int ROI_START(int x) {return 1;}
  int ROI_END(int x) {return 1;}

  int SIM_BEGIN(int x) {return 1;}
  int SIM_END(int x) {return 1;}
}

void run() {
  ROI_START(1);
  printf("Hello!!\n");
  ROI_END(1);
}


int main (int argc, char** argv) {
  SIM_BEGIN(1);
  run();
  SIM_END(1);
/* print_graph(); */
  return 0;
}
