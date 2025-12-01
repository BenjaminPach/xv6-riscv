cat > INFORME.md << 'EOF'
# INFORME — Tarea xv6-riscv: nuevas syscalls `getppid()` y `getancestor(n)` + programa de prueba

**Alumno:** _[escribe tu nombre]_  
**Curso/Sec:** _[opcional]_  
**Fecha:** _[opcional]_  

---

## 1. Objetivo de la tarea
Implementar en **xv6-riscv** dos llamadas al sistema:

- **`getppid()`**: retorna el PID del proceso padre del proceso actual; si no existe, retorna **-1**.
- **`getancestor(n)`**: retorna el PID del ancestro **n** del proceso actual:
  - `n = 0` → el PID del proceso actual,
  - `n = 1` → el PID del padre,
  - `n ≥ 2` → se asciende por la cadena de padres; si no existe tal ancestro, retorna **-1**.

Además, crear el programa de usuario **`yosoytupadre`** para comprobar el correcto funcionamiento.

---

## 2. Entorno y dependencias
- **Host:** macOS (MacBook Air)  
- **Toolchain RISC-V:** `riscv64-unknown-elf-gcc`, `ld`, `objdump`  
- **Emulador:** `qemu-system-riscv64` (v7.2 o superior)  
- **Edición:** VS Code (para editar), **Terminal de macOS** (para compilar/ejecutar).

---

## 3. Diseño y decisiones
- **Semántica:**  
  - `getppid()` devuelve el PID del padre si existe; en caso contrario `-1`.  
  - `getancestor(n)` valida `n`. Si `n<0`, retorna `-1`. Para `n>=0`, parte en el proceso actual y sube `n` veces por `parent`. Si en el camino se queda sin padre, retorna `-1`.
- **Concurrencia/bloqueos:** lectura simple de `pid` y `parent` como hace `sys_getpid` en xv6; no se modifica estado, por lo que no se añadieron locks adicionales.
- **Interfaz de usuario:** programa `yosoytupadre` muestra `PID`, `PPID`, los `ancestor(i)` para `i=0..4`, y crea un hijo para verificar consistencia en ambos procesos.

---

## 4. Cambios realizados (archivos y código)

### 4.1 `kernel/syscall.h` — números de syscalls
> **Agregar al final**:
```c
// NUEVOS:
#define SYS_getppid     22
#define SYS_getancestor 23
