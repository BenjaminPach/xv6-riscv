
#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"

int main(int argc, char *argv[]) {
  int N = 10;
  settickets(1);
  

  for (int i = 0; i < N; i++) {
    int pid = fork();
    if (pid < 0) { printf("fork failed\n"); exit(1); }
    if (pid == 0) {
      int myt = 50 * (i + 1);   // 50, 100, 150, ...
      settickets(myt);
      volatile unsigned long s = 0;
      for (;;) { for (int k = 0; k < 1000000; k++) s += k; }
      
    }
  }
  sleep(200);
  exit(0);
}

