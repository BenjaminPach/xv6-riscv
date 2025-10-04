// user/orphantest.c
#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  int pid = fork();
  if(pid < 0){
    printf("fork failed\n");
    exit(1);
  }
  if(pid == 0){
    // Hijo: espera a que el padre muera y verifica nuevo PPID
    sleep(10); // ~10 ticks
    printf("[child] pid=%d ppid=%d (esperado 1)\n", getpid(), getppid());
    exit(0);
  } else {
    // Padre: muere ahora para dejar huérfano al hijo
    exit(0);
  }
}
