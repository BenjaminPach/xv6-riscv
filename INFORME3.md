
---

## Resumen

Se implementaron dos nuevas llamadas al sistema, `mrdprotect(void *addr, int len)` y `munrdprotect(void *addr, int len)`, que permiten a un proceso marcar un rango de páginas de su espacio de direcciones como **no legibles** y luego revertir esa protección. La motivación principal es poder almacenar datos sensibles (por ejemplo llaves criptográficas) en memoria de usuario que no puedan ser leídos de vuelta; cualquier intento de acceso sobre esa región provoca un **page fault** y termina el proceso.

A nivel conceptual, el mecanismo funciona modificando los **bits de permisos de las entradas de tabla de páginas (PTE)** del proceso. El parámetro `addr` debe estar alineado a página y `len` indica cuántas páginas consecutivas se afectan. Para cada página del rango se verifica que realmente pertenezca al espacio de usuario, que esté mapeada y marcada como de usuario; sólo en ese caso se modifica el bit de lectura (`PTE_R`). En caso de error (dirección fuera de rango, página no mapeada, etc.) la operación falla y retorna `-1`.

**Puntos esenciales:**

* Nuevas syscalls de usuario: `mrdprotect` y `munrdprotect`.
* Protección a nivel de **página completa**, usando el bit `PTE_R` en los PTE.
* Chequeos de robustez: alineación de `addr`, `len > 0`, dirección dentro de `p->sz`, `PTE_V` y `PTE_U`.
* En éxito retornan `0`; en cualquier condición inválida retornan `-1`.
* Al acceder a una página protegida se observa un **page fault** en `usertrap()` con `va` igual a la dirección protegida (en las pruebas, `0x4000`).

---

## Resumen de la implementación

Desde user space, las funciones `mrdprotect`/`munrdprotect` se definen en `user/user.h` y sus stubs se generan en `user/usys.S` (a través de `usys.pl`), de modo que un `ecall` invoque el número de syscall correspondiente. En el kernel, `sys_mrdprotect` y `sys_munrdprotect` (en `kernel/sysproc.c`) usan `argaddr`/`argint` para recuperar `addr` y `len` desde la traza de sistema, y delegan la lógica principal a `mrdprotect()` y `munrdprotect()` implementadas en `kernel/vm.c`. Estas funciones obtienen el `pagetable_t` del proceso actual (`myproc()`), recorren las `len` páginas a partir de `addr` verificando que cada dirección virtual esté dentro de `p->sz` y que el PTE devuelto por `walk()` sea válido (`PTE_V`) y de usuario (`PTE_U`). En `mrdprotect` se limpia el bit `PTE_R` de cada PTE, en `munrdprotect` se vuelve a activar; tras actualizar los PTE se invoca `sfence_vma()` para invalidar el TLB y asegurar que los nuevos permisos surtan efecto inmediato.

---

## Cambios de código

### 1. Interfaz de usuario y stubs de syscalls

* **`user/user.h`**

  * Se agregaron las firmas:

    * `int mrdprotect(void *addr, int len);`
    * `int munrdprotect(void *addr, int len);`

* **`user/usys.pl` / `user/usys.S`**

  * Se añadieron las entradas para generar los stubs:

    * `entry("mrdprotect");`
    * `entry("munrdprotect");`

### 2. Números de syscall y tabla del kernel

* **`kernel/syscall.h`**

  * Definición de nuevos IDs:

    * `#define SYS_mrdprotect  XX`
    * `#define SYS_munrdprotect YY`
    * (usando los números consecutivos libres del archivo).

* **`kernel/syscall.c`**

  * Declaraciones:

    * `extern uint64 sys_mrdprotect(void);`
    * `extern uint64 sys_munrdprotect(void);`
  * Asociación en la tabla de syscalls:

    * `[SYS_mrdprotect]  sys_mrdprotect,`
    * `[SYS_munrdprotect] sys_munrdprotect,`

### 3. Wrappers de syscalls y lógica de VM

* **`kernel/defs.h`**

  * Prototipos visibles para otros módulos:

    * `int mrdprotect(void *addr, int len);`
    * `int munrdprotect(void *addr, int len);`

* **`kernel/sysproc.c`**

  * Nuevos wrappers:

    * `sys_mrdprotect()` y `sys_munrdprotect()` que:

      * Llaman a `argaddr(0, &addr)` y `argint(1, &len)`.
      * Retornan el resultado de `mrdprotect((void*)addr, len)` / `munrdprotect((void*)addr, len)`.

* **`kernel/vm.c`**

  * Implementación de:

    * `mrdprotect(void *addr, int len)`:

      * Valida `len > 0` y que `addr` esté alineada a `PGSIZE`.
      * Recorre las `len` páginas verificando `cur_va < p->sz`, `walk(pagetable, cur_va, 0) != 0`, `PTE_V` y `PTE_U`.
      * Borra el bit `PTE_R` en cada PTE válido.
    * `munrdprotect(void *addr, int len)`:

      * Misma lógica de validación, pero **setea** el bit `PTE_R`.
      * Ambas funciones invocan `sfence_vma()` al final.

### 4. Programa de prueba y Makefile

* **`user/rdprotect_test.c`** (archivo nuevo)

  * Reserva una página con `sbrk`.
  * Escribe en `addr[0]` antes de proteger (control).
  * Invoca `mrdprotect(addr, 1)`.
  * Realiza un acceso a la página protegida (lectura/escritura en `addr[0]`), que provoca el page fault.
  * Opcionalmente podría llamar a `munrdprotect` para revertir y volver a leer, pero en la versión final se deja el acceso fallido para evidenciar la protección.

* **`Makefile`**

  * Se añadió `_rdprotect_test` a la variable `UPROGS` para que el ejecutable quede disponible en el shell de xv6.

---

## Pruebas realizadas

Para compilar y ejecutar el sistema modificado se usó:

```bash
make clean
make qemu
```

En el prompt de xv6 se ejecutó repetidamente:

```bash
$ rdprotect_test
usertrap(): page fault pid=3 sepc=0x2c va=0x4000
$ rdprotect_test
usertrap(): page fault pid=4 sepc=0x2c va=0x4000
...
```

La dirección virtual que falla (`va=0x4000`) coincide con la página asignada por `sbrk`, y `sepc` apunta a la instrucción que accede a `addr[0]`. La repetición del patrón con distintos `pid` y la correcta ejecución del shell después del fallo muestran que el mecanismo de protección está funcionando como se espera: **al intentar acceder a una página marcada por `mrdprotect`, el proceso recibe un page fault y es terminado por el kernel**.

También se probó comentar temporalmente la llamada a `mrdprotect` dentro de `rdprotect_test.c`; en ese caso el programa termina normalmente sin page faults, lo que confirma que la falla observada se debe exclusivamente a la modificación de permisos en los PTE.

---

## Dificultades encontradas y soluciones

* **Interpretación de los page faults (`scause`)**
  Al principio se observaron mensajes del tipo `usertrap(): unexpected scause 0xf`, que corresponden a *store page fault*. Esto ocurría cuando el primer acceso a la página protegida era una escritura (`addr[0] = 'A';`). Se verificó contra la especificación de RISC-V que, al limpiar `PTE_R`, la página queda en un estado en que cualquier acceso puede provocar fault, por lo que es esperable ver tanto load como store page faults. Se ajustó el programa de prueba y se documentó este comportamiento en el informe.

* **Errores al registrar las syscalls**
  Durante el desarrollo aparecieron errores de link al olvidar alguna de las etapas al agregar nuevas syscalls (definición del número en `syscall.h`, entrada en la tabla `syscalls[]`, stub en `usys.pl`). Se solucionó siguiendo exactamente el patrón de las syscalls existentes y verificando con `grep SYS_mrdprotect` / `SYS_munrdprotect` que estuvieran presentes en todos los archivos requeridos.


