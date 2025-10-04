#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int pid = getpid();
  int ppid = getppid();
  printf("PID=%d PPID=%d\n", pid, ppid);

  // Muestra varios niveles de ancestros
  for (int k = 0; k <= 4; k++) {
    int a = getancestor(k);
    printf("ancestor(%d) = %d\n", k, a);
  }

  // Probar jerarquía con un fork
  int c = fork();
  if (c < 0) {
    printf("fork failed\n");
    exit(1);
  }
  if (c == 0) {
    // Hijo
    printf("[child] pid=%d ppid=%d a1=%d a2=%d\n",
           getpid(), getppid(), getancestor(1), getancestor(2));
    exit(0);
  } else {
    // Padre
    wait(0);
  }

  exit(0);
}
