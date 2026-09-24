# Reporte – Simulador de Memoria Virtual (FIFO)

## 1. Estructuras de datos
| Estructura | Descripción |
|---|---|
| Dirección virtual (32 bits) | `PT1` = bits 31-22, `PT2` = bits 21-12, `offset` = bits 11-0. Se extraen con desplazamientos y máscaras. |
| `PTE` | `frame` (# página física), `valid`, `accessed`, `dirty` y `allocated` (la página pertenece a un `alloc`; permite detectar accesos ilegales). |
| `Level2Table` | Arreglo de 1024 `PTE`. Se crea con `new` **solo cuando se necesita** (al hacer `alloc`). |
| `level1[1024]` | Arreglo de punteros a `Level2Table` (nulo = tabla inexistente). Se libera en el destructor. |
| `physMem` | `vector<uint8_t>` que simula la RAM (256 KB por defecto = 64 marcos de 4 KB). |
| `freeFrames` | Pila de marcos libres. |
| `frameOwner` | Para cada marco, qué página virtual lo ocupa (necesario para invalidar la PTE de la víctima). |
| `fifoQueue` | `deque` de marcos en orden de carga; el frente es el más antiguo. |
| `swapSpace` | `map<VPN, contenido>` que simula el disco: guarda páginas modificadas expulsadas. |
| `regions` | `map<VA inicial, #páginas>` para poder ejecutar `free`. |

**Flujo de una traducción** (`translate`): se separa la VA → se busca la PTE por los 2 niveles → si no está reservada, error → si `valid`, es un *hit* → si no, *fallo de página* (`handlePageFault`) → se obtiene el marco → se calcula `PA = frame * 4096 + offset`.

Las funciones están separadas como pide el enunciado: `translate` (traducción), `handlePageFault` (fallos), `selectVictimFIFO` + `evictFrame` (reemplazo).

## 2. Política de reemplazo: FIFO
Cada vez que una página se carga en un marco, ese marco entra al final de `fifoQueue`. Cuando ocurre un fallo y no hay marcos libres, la víctima es el marco del **frente** de la cola (la página que lleva más tiempo en memoria), sin importar cuánto se haya usado. Los *hits* no modifican la cola. Si la víctima está `dirty`, su contenido se guarda en `swapSpace` para poder recuperarlo después (verificado: escribir 123, expulsar la página y releerla devuelve 123). Es determinista y de costo O(1).

## 3. Resultados (256 KB = 64 marcos)
| Prueba | Accesos | Fallos | Hit rate | Reemplazos |
|---|---|---|---|---|
| 1. Básico (ejemplo del enunciado) | 4 | 2 | 50.00 % | 0 |
| 2. Secuencial: 100 págs × 2 pasadas | 200 | 200 | 0.00 % | 136 |
| 3. Localidad: 2000 accesos, 80 % a 20 págs "calientes" | 2000 | 236 | 88.20 % | 172 |

Reproducir: `./vmsim tests/testN_*.txt` (el test 3 se genera con semilla fija en `tests/generar_pruebas.py`).

## 4. Análisis
- **Prueba 1:** las dos primeras referencias son fallos obligatorios (*cold misses*, paginación por demanda); las relecturas son hits. No hay reemplazos porque todo cabe.
- **Prueba 2:** 100 páginas no caben en 64 marcos. Con FIFO, al llegar a la página 64 se empieza a expulsar justo las más antiguas, que son las que se necesitarán primero en la segunda pasada, así que **todo acceso es fallo** (0 % de hit rate). Con 512 KB (`-m 512`) las 100 páginas caben y el hit rate sube a 50 % (solo fallan las primeras 100), sin reemplazos.
- **Prueba 3:** con localidad, el hit rate es alto (88.2 %). Sin embargo, FIFO sigue expulsando páginas calientes solo por ser "viejas": las 172 reemplazos incluyen páginas que se vuelven a pedir enseguida.
- Los fallos totales se descomponen en 100 obligatorios (páginas distintas tocadas) + reemplazos dependientes de la política.

## 5. Comparación teórica con LRU
LRU expulsa la página **no usada hace más tiempo**, por lo que aprovecha la localidad temporal: una página caliente se "renueva" con cada acceso y no se expulsa. FIFO ignora el uso. Costo: LRU necesita actualizar el orden en cada acceso (lista + tabla hash, o bits aproximados como el algoritmo del reloj), FIFO solo en cada fallo.

Simulamos LRU sobre las mismas trazas (script auxiliar, no forma parte del entregable):

| Prueba | Fallos FIFO | Fallos LRU |
|---|---|---|
| 2. Secuencial | 200 | 200 |
| 3. Localidad | 236 | 174 |

- En el recorrido cíclico mayor que la memoria, **LRU tampoco ayuda** (también expulsa justo la página que se usará después); es el peor caso de ambas.
- Con localidad, LRU reduce ~26 % los fallos (236 → 174).
- **Anomalía de Belady:** FIFO no es un algoritmo de pila; con la traza clásica `1 2 3 4 1 2 5 1 2 3 4 5` FIFO produce 9 fallos con 3 marcos y **10 con 4**, más memoria puede empeorar el resultado. LRU (8 fallos con 4 marcos) no sufre esta anomalía.

## 6. Calidad
- Compila con `g++ -Wall -Werror -std=c++11` sin warnings.
- Sin fugas ni errores de memoria: verificado con AddressSanitizer/LeakSanitizer/UBSan en todas las pruebas. **Pendiente para ustedes:** correr `valgrind --leak-check=full ./vmsim tests/test3_localidad.txt` en su máquina y pegar el resultado aquí.
