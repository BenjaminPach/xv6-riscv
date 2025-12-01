Se implemento un scheduler por loteria la idea es que cada proceso tenga cierta cantidad de tickets y el scheduler “rifa” quién corre: más tickets ⇒ mayor probabilidad de ser elegido. Además, conté cuántas veces fue elegido cada proceso (cpu_slices) para verificar que, en promedio, la CPU se reparte proporcionalmente a los tickets.

En cada iteración del scheduler():
Recorro la tabla de procesos y sumo los tickets de todos los RUNNABLE (forzando mínimo 1 por robustez).
Sorteo un número aleatorio r entre 1 y total.
Hago una pasada acumulando tickets hasta que acc >= r: ese proceso es el ganador.
Lo pongo RUNNING, incremento cpu_slices, hago swtch() y al volver retorno al loop.
Usé un PRNG simple (LCG) en kernel solo para el sorteo (no criptográfico, suficiente para scheduling).

archivos modificados:

	modificados:     .DS_Store
	modificados:     INFORME.md
	modificados:     Makefile
	modificados:     kernel/proc.c 
        allocproc(): inicializo p->tickets = 100; p->cpu_slices = 0;.
        kfork(): el hijo hereda tickets del padre y arranca con cpu_slices = 0. (Para demos más estables se puede setear 100 fijo en vez de heredar).
       


	modificados:     kernel/proc.h
     se agrego:
            int tickets;      // tickets de lotería (>=1)
            int cpu_slices;   // # de veces que el scheduler lo eligió
	modificados:     kernel/syscall.c
	modificados:     kernel/syscall.h
	modificados:     kernel/sysproc.c
        se agregó: 
        uint64 sys_settickets(void) {
             int n; if (argint(0,&n) < 0) return -1;
            if (n < 1) n = 1;
             myproc()->tickets = n;
            return 0;
            }           

	modificados:     user/user.h
	modificados:     user/usys.pl

archivos nuevos:
	nuevos archivos: INFORME2.md
	nuevos archivos: user/demo.c
        Este funciona asi: Creo N hijos; cada hijo llama settickets(50*(i+1)) (50, 100, 150, …, 500) y hace carga ocupada para acumular CPU. El padre se baja a settickets(1) para no estorbar. Probé también una versión que termina sola y espera a los hijos.



en SYSCALL
agregué settickets(int n) para que el usuario pueda ajustar sus ticketgs:
user/usys.pl: añadí entry("settickets"); para generar el stub.
user/user.h: añadí int settickets(int n);
kernel/syscall.h: definí #define SYS_settickets <N> (usé el siguiente número libre).
kernel/syscall.c: extern uint64 sys_settickets(void); y en la tabla: [SYS_settickets] sys_settickets,.
kernel/sysproc.c:



Dificultades encontradas y soluciones implementadas 
“make clean no existe”: ejecuté make en ~ en vez del repo. Solución: cd ~/xv6-riscv (raíz) y recién ahí make clean && make qemu.
exec $ failed: intenté ejecutar “$ demo”. El “$” es el prompt, no va en el comando. Usé solo demo.
Faltaba _demo: el Makefile pedía user/_demo y me tiró “No rule to make target user/_demo…”. Creé user/demo.c y agregué _demo en UPROGS.
No había usys.S: en mi fork se genera desde user/usys.pl. Agregué entry("settickets"); y recompilé (eso crea user/usys.S).
Heredoc pegado en el fuente: pegué cat > user/demo.c <<'EOF' dentro del archivo por error, el compilador explotó con cosas raras (unknown type name 'uint', etc.). Lo corregí recreando el archivo desde la terminal (el heredoc se escribe en la terminal, no dentro del .c).
Slices “raros” al principio:
El padre se bajaba a 1 antes del fork(), y en kfork() el hijo hereda ⇒ los hijos nacían con 1 y, por baja probabilidad, algunos no alcanzaban a correr su propio settickets(más alto).
Arreglé moviendo settickets(1) del padre después de crear los hijos (o, alternativamente, seteando np->tickets=100 en kfork() para demos).
También corrí con CPUS=1 para reducir variabilidad.



Posibles problemas del lottery scheduling
Alta varianza en ventanas cortas: al ser probabilístico, en pocos sorteos puede “no parecer” proporcional.
Mitigación: observar ventanas más largas, o usar 1 CPU.
Starvation poco probable pero posible: con tickets muy desbalanceados, procesos con pocos tickets pueden tardar mucho en ser elegidos.
Mitigación: imponer mínimos (yo forcé tickets>=1) o combinar con prioridades/aging.
Sensibilidad a tamaño de quantum: si el time slice es largo, la aleatoriedad pega más.
Mitigación: cuantums razonables y observación en ventanas más amplias.
Semilla/PRNG simple: usé un LCG básico; si uno quisiera repetir experimentos con exactitud, habría que semillar y/o mejorar el PRNG (no crítico para la tarea).
