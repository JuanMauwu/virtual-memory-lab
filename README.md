# Simulador de Memoria Virtual (paginación de 2 niveles, reemplazo FIFO)

Laboratorio de memoria virtual. Lenguaje: **C++11**. Política de reemplazo: **FIFO**.

## Compilación
```
make          # genera el ejecutable ./vm del con el makefile 
make run      # ejecuta tests/test1_basico.txt en modo detallado (-v)
make clean
```

## Uso: ejemplo con datos de testeo 1
```
./vmsim test/test1_basico.txt
./vmsim test/test1_basico.txt [-m KB] [-v]
./vmsim test/test1_basico.txt [-v]
./vmsim test/test1_basico.txt [-m KB]
./vmsim test/test1_basico.txt [-m KB] [-v] 
```
- `-m KB`: memoria física en KB (defecto y mínimo 256).
- `-v`: imprime cada operación con su PA y si fue `[FALLO]` o `[HIT]`.

### Formato del archivo de entrada
Comandos separados por espacios o saltos de línea:
```
alloc <bytes>            reserva memoria virtual (empieza en VA 0, se redondea a páginas de 4KB)
write <virtual_addr> <v> escribe 1 byte (0-255) en la VA
read  <virtual_addr>     lee 1 byte de la VA
free  <virtual_addr>     libera la región que empieza en esa VA
```
Las direcciones aceptan decimal o hexadecimal (`0x1000`). Acceder a una dirección no reservada imprime un error (segfault simulado) y **no** cuenta como acceso.

## Estructura
```
src/vm.h      estructuras (PTE, Level2Table, Stats) y clase VirtualMemory
src/vm.cpp    traducción, manejo de fallos, reemplazo FIFO, alloc/free
src/main.cpp  lectura del archivo y estadísticas
tests/        programas de prueba (+ generar_pruebas.py)
REPORTE.md    reporte de análisis
```

## Nota
El enunciado menciona `gcc -std=c99`; al usar C++ compilamos con `g++ -Wall -Werror -std=c++11`, con la misma exigencia de cero warnings.
