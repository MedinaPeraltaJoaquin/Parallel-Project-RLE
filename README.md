# Parallel-Project-RLE: Compresión y Descompresión RLE Paralela (MPI)

Creado por Medina Peralta Joaquín

Este proyecto implementa un compresor/descompresor Run-Length Encoding (RLE) utilizando paralelismo a través de **Message Passing Interface (MPI)**. El objetivo principal es evaluar el Speedup y la eficiencia de la compresión/descompresión cuando se utilizan múltiples procesadores.

Para ver la documentación completa y el análisis de resultados, consulte el [Informe Técnico en PDF](docs/main.pdf).

---

## Instrucciones de Compilación y Ejecución

El proyecto utiliza un `Makefile` para simplificar la gestión de la compilación y las pruebas. Se requiere tener instalado el compilador de MPI (`mpicxx`, como OpenMPI o MPICH).

### Requisitos

* Un compilador C++17.
* Librería MPI instalada (ej. OpenMPI, MPICH).
* Herramienta `make`.

### Comandos de Compilación

Para compilar el ejecutable principal (`rle_compressor`) y todos los binarios de prueba:

```bash
make all
```

Los binarios se crearán en el directorio `build/`

### Comandos de ejecución

El ejecutable principal es `build/rle_compressor`. Para ejecutarlo, se
debe usar el comando `mpirun` y especificar el número de procesos usando
la banera `-np N` .

| Tarea | Comando en Makefile | Comando Manual |
| --- | --- | --- |
| Ejecutar con N procesos | `make run NP=N ARGS="<args>"` | `mpirun -np N ./build/rle_compressor <args>` |
| Ejecutar Benchmark | `make benchmark` | Ejecuta el script `run_benchmarks.sh` para obtener todos los resultados de tiempo |
| Generar Datos de Prueba | `make generate_data` | Ejecuta el script `generate_test_data.sh` para crear los archivos de 100MB. |
| Limpiar | `make clean` | Elimina el directorio build/ y los archivos de salida comprimidos (*.rle).|
| Limpiar los datos de prueba | `make clean_data` | Elimina los archivos de prueba (`*.bin`, `*.rle`, `*.csv`) y archivos temporales (`temp_log_*.txt`)|
| Realizar tests unitarios y de integración | `make test*` | Compilar los tests y ejecutar cada binario. |

Para ejecutar los tests del proyecto, se tienen las siguientes pruebas:

| Tarea | Comando en Makefile |
| --- | --- |
| Ejecuta prueba de integración de compresión secuencial | `make test_sequential` |
| Ejecuta prueba de frontera de `RLE` (solo compresión) | `make test_boundary` |
| Ejecuta prueba de integración de compresión y descompresión `RLE` | `make test_all_boundary` | 
| Ejecuta prueba de lectura y división correcta del archivo | `make test_mpi_io` |
| Ejecuta pruebas unitarias para compresión y descompresión local de `RLE` | `make test` |

## Explicación del Paralelismo

El paralelismo se implementa utilizando el modelo Master-Slave
(Maestro-Esclavo) sobre la interfaz MPI para la distribución de la carga de 
trabajo.

### Estrategia de División de Tareas

* División de Bloques (Chunking)

    - El proceso Maestro (`rank 0`) calcula el tamaño total del archivo de entrada que puede ser comprimido o sin comprimir.

    - El archivo se divide en `N` chunks de tamaño aproximadamente igual, donde `N` es el número de procesos utilizados.

* Uso de MPI-I/O (E/S Distribuida):

    - En lugar de que el Maestro lea todo el archivo y luego envíe los bloques a los Esclavos (lo que sería ineficiente por el uso de memoria y comunicación P2P), se utiliza MPI-I/O (MPI_File_read_at).

    - Cada proceso lee directamente la porción asignada del archivo desde el disco.

### Manejo de Fronteras (Descompresión - Crítico)

La compresión RLE utiliza códigos de longitud variable. El principal desafío en la descompresión paralela es asegurar que un token RLE no quede dividido entre el final de un bloque y el inicio del siguiente.

* Solapamiento: Cada proceso $P_i$ para $i > 0$ lee 2 bytes adicionales del final del bloque $P_{i−1}$​. Este solapamiento garantiza que, si un token RLE (ej. FLAG + CONTEO + BYTE) está partido, el proceso $P_i$​ obtenga los bytes de inicio del token.

* Recorte: Después de la decodificación local, el proceso $P_i$​ ejecuta una revisión ed frontera con el proceso continuo: simula la decodificación solo de la región de solapamiento para calcular el número de bytes descomprimidos que corresponden a la redundancia leída. Estos bytes redundantes se eliminan del buffer de salida de $P_i$​ para asegurar la concatenación correcta de los datos.

### Recolección de Resultados

* Cada proceso calcula la longitud de su segmento descomprimido (local_len).

* El Maestro recolecta todas las longitudes (MPI_Gather) y calcula el tamaño final del archivo.

* Finalmente, el Maestro utiliza MPI_Gatherv para recolectar todos los segmentos de salida descomprimida en un único buffer grande antes de escribirlo en el disco.

## Parámetros de Entrada/Salida

El programa `rle_compressor` soporta dos modos de operación que se definen
por los argumentos de la línea de comandos:

### Uso general

``` bash
mpirun -np <N> ./build/rle_compressor <MODE> <INPUT_FILE> [OPTIONS] --output <OUTPUT_FILE>
```
Con la información de los parámetros como:

| Parámetro | Descripción |
| --- | --- |
| `<N>` | Número de Procesos MPI. Determina el grado de paralelismo.|
| `<MODE>` | Modo de Operación. Puede ser `--compress` o `--decompress`.|
|`<INPUT_FILE>` | "Ruta al archivo de origen (ej. `.bin` para compresión, .`rle` para descompresión)."|
| `<OUTPUT_FILE>` | "Ruta donde se escribirá el resultado (ej. `.rle` para compresión, `.bin` para descompresión)."|
| `[OPTIONS]` | Opciones de ejecución siendo `--secuencial` que ejecuta la versión secuencial y `--parallel` que ejecuta la versión paralela| 

### Ejemplo de compresión y descompresión paralela con 4 procesos

``` bash
mpirun -np 4 ./build/rle_compressor --compress --output test_data/data_aleatoria.bin output/aleatoria.rle
```

```bash
mpirun -np 4 ./build/rle_compressor --decompress --output /aleatoria.rle output/aleatoria_final.bin
```