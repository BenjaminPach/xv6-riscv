#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(void)
{
  char *addr = sbrk(0); // Dirección actual del heap
  sbrk(4096);           // Reservar una página (PGSIZE)

  addr[0] = 'Z'; // Escribir valor inicial

  // Proteger contra lectura (en RISC-V también bloquea escritura)
  if (mrdprotect(addr, 1) < 0) {
    printf("mrdprotect falló\n");
    exit(1);
  }

  // Intento de lectura debería provocar fallo
  char c = addr[0];
  printf("Valor leído: %c (esto NO debería imprimirse)\n", c);

  // Revertir protección
  if (munrdprotect(addr, 1) < 0) {
    printf("munrdprotect falló\n");
    exit(1);
  }

  printf("Protección revertida correctamente.\n");
  exit(0);
}
