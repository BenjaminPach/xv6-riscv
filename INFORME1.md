**Benjamín López**

> **Resumen.** Se implementan dos syscalls nuevas en xv6-riscv: `getppid()` (retorna el PID del padre) y `getancestor(int n)` (retorna el PID del ancestro *n*-ésimo, o `-1` si no existe). Se incluyen los cambios en kernel y user space, un programa de prueba, notas para compilar/ejecutar y tips útiles.

---

## Funcionalidad implementada
- `getppid()`: retorna el PID del proceso padre del proceso actual.
- `getancestor(int n)`: retorna el PID del ancestro *n*-ésimo del proceso actual (0 = el propio proceso, 1 = su padre, 2 = su abuelo, ...). Si el ancestro no existe, retorna `-1`.

---

## Archivos modificados (kernel y user)

> **Nota sobre numeración de syscalls:** usa **números libres** según tu `kernel/syscall.h`. A modo de ejemplo dejo `22` y `23`. Si están ocupados, elige los siguientes disponibles.

### 1) `kernel/syscall.h` — definición de números
```c
// Agregar al final de las definiciones existentes:
#define SYS_getppid      22   // usa un número libre
#define SYS_getancestor  23   // usa un número libre diferente
2) kernel/sysproc.c — lógica de las syscalls
#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_getppid(void) {
  struct proc *p = myproc();
  if (p->parent)
    return p->parent->pid;
  return -1; // por ejemplo, init no tiene padre "normal"
}

uint64
sys_getancestor(void) {
  int n;
  if (argint(0, &n) < 0 || n < 0)
    return -1;

  struct proc *p = myproc();
  // n = 0 => el propio proceso
  for (int i = 0; i < n; i++) {
    if (p->parent == 0)
      return -1;
    p = p->parent;
  }
  return p->pid;
}
3) kernel/syscall.c — prototipos + registro en la tabla
// Prototipos (cerca de los demás extern):
extern uint64 sys_getppid(void);
extern uint64 sys_getancestor(void);

// En la tabla syscalls[]:
[SYS_getppid]      sys_getppid,
[SYS_getancestor]  sys_getancestor,
4) user/user.h — prototipos en user space
int getppid(void);
int getancestor(int n);
5) user/usys.pl — entrada para generar stubs
entry("getppid");
entry("getancestor");
Programa de prueba
user/yosoytupadre.c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int pid = getpid();
  int ppid = getppid();
  printf("Mi PID: %d, Mi padre: %d\n", pid, ppid);

  // Probar distintos niveles de ancestros
  for (int i = 0; i < 5; i++) {
    int a = getancestor(i);
    printf("getancestor(%d) = %d\n", i, a);
  }

  // Prueba adicional: crear un hijo y ver su padre/ancestros
  int c = fork();
  if (c < 0) {
    printf("fork() fallo\n");
    exit(1);
  } else if (c == 0) {
    // Hijo
    int hpid = getpid();
    int hppid = getppid();
    printf("[Hijo] pid=%d, ppid=%d, a1=%d, a2=%d\n",
           hpid, hppid, getancestor(1), getancestor(2));
    exit(0);
  } else {
    // Padre
    wait(0);
  }

  exit(0);
}
Compilación y ejecución
Makefile (user) — agregar el binario
UPROGS += $U/_yosoytupadre
Construir y correr xv6
make clean
make qemu
# En el shell de xv6:
yosoytupadre
Salida esperada (ejemplo):
Mi PID: 3, Mi padre: 2
getancestor(0) = 3
getancestor(1) = 2
getancestor(2) = 1
getancestor(3) = -1
getancestor(4) = -1
[Hijo] pid=4, ppid=3, a1=3, a2=2
Dificultades y solución
Símbolos no declarados en syscall.c:
'sys_getppid' undeclared here (not in a function)
'sys_getancestor' undeclared here (not in a function)
Solución:
Asegurar extern en kernel/syscall.c.
Registrar en syscalls[] como se muestra arriba.
