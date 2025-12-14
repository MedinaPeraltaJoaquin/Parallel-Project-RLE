#include "../include/RLECompressor.hpp"
#include <iostream>
#include <string>
#include <mpi.h>

using namespace std;

// Función para mostrar el uso del programa
void show_usage(const string& name) {
    cerr << "Uso: " << name << " <archivo_entrada> [OPCIONES]" << endl
         << "Opciones:" << endl
         << "  --secuencial  Ejecuta la versión secuencial (solo rank 0)" << endl
         << "  --parallel    Ejecuta la versión paralela (predeterminado)" << endl
         << "  --output <file> Especifica el nombre del archivo de salida sin extensión." << endl
         << endl;
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 2) {
        if (rank == 0) show_usage(argv[0]);
        MPI_Finalize();
        return 1;
    }

    string input_file = argv[1];
    string output_file = input_file + ".rle";
    bool sequential_mode = false;

    for (int i = 2; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--secuencial") {
            sequential_mode = true;
        } else if (arg == "--parallel") {
            sequential_mode = false;
        } else if (arg == "--output" && i + 1 < argc) {
            output_file = argv[++i];
        }
    }

    if (sequential_mode) {
        if (rank == 0) {
            cout << "  - Ejecutando: Compresion RLE Extendido Secuencial" << endl;
            //RLECompressor::RunSequential(input_file, output_file);
        }
    } else {
        if (rank == 0) {
            cout << "  - Ejecutando: Compresion RLE Extendido Paralelo" << endl;
        }
        //RLECompressor::RunParallel(input_file, output_file, rank, size);
    }

    MPI_Finalize();
    return 0;
}