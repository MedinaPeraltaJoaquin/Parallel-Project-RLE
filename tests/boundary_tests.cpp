#include "../include/RLECompressor.hpp"
#include <iostream>
#include <vector>
#include <mpi.h>
#include <cassert>
#include <numeric>
#include <fstream>
#include <cstdio>  
#include <cstring>
#include <string>

using namespace std;

// Parámetros de la Prueba
const string INPUT_FILE = "test_data/boundary_in.bin";
const string OUTPUT_FILE = "test_data/boundary_out.rle";

// --- Casos de Prueba Críticos ---
enum TestCases {
    CASE_RLE_B_PURE = 0,     // Secuencia pura RLE (BBBBBB), prueba de fusión esencial.
    CASE_LITERAL_TO_RLE = 1, // Secuencia AABBB, la fusión ocurre en el límite (B-B).
    CASE_RLE_TO_LITERAL = 2, // Secuencia BBBAA, NO ocurre fusión (B != A).
    CASE_RLE_A_PURE = 3      // Secuencia pura RLE (AAAAAA), prueba de fusión con otra letra.
};

// Mapa para la impresión de resultados
const char* test_case_names[] = {
    "RLE_Puro (BBBBBB)", 
    "Literal a RLE (AABBB)", 
    "RLE a Literal (BBBAA)",
    "RLE_Puro (AAAAAA)"
};

// --- Función para crear el archivo de prueba (Solo Rank 0) ---
vector<uint8_t> create_test_file(const string& file_name, TestCases test_case) {
    vector<uint8_t> data;
    
    switch (test_case) {
        case CASE_RLE_B_PURE:
            // Entrada: BBBBBB (6 bytes). Esperado: FF 06 42
            data = {66, 66, 66, 66, 66, 66}; 
            break;
            
        case CASE_LITERAL_TO_RLE:
            // Entrada: AABBB (5 bytes). Esperado: 41 41 FF 03 42
            data = {65, 65, 66, 66, 66};
            break;
            
        case CASE_RLE_TO_LITERAL:
            // Entrada: BBBAA (5 bytes). Esperado: FF 03 42 41 41
            data = {66, 66, 66, 65, 65};
            break;

        case CASE_RLE_A_PURE:
            // Entrada: AAAAAA (6 bytes). Esperado: FF 06 41
            data = {65, 65, 65, 65, 65, 65};
            break;
    }

    ofstream ofs(file_name, ios::binary);
    if (!ofs) {
        cerr << "Error al crear el archivo de prueba: " << file_name << endl;
        return {};
    }
    ofs.write((const char*)data.data(), data.size());
    ofs.close();
    
    return data;
}

// --- Función de Prueba de Integración de Fronteras con Verificación Exacta ---
void run_boundary_test(int rank, int size) {
    if (size < 2) {
        if (rank == 0) cout << "Se requieren al menos 2 procesos para la prueba de fronteras." << endl;
        return;
    }

    for (int t = 0; t <= CASE_RLE_A_PURE; ++t) {
        TestCases test_case = (TestCases)t;
        vector<uint8_t> original_data;

        if (rank == 0) {
            cout << "\n========================================================================" << endl;
            cout << ">>> EJECUTANDO CASO DE PRUEBA: " << test_case_names[t] << " <<<" << endl;
            cout << "========================================================================" << endl;
            
            // 1. Crear el archivo de prueba para el caso actual
            original_data = create_test_file(INPUT_FILE, test_case);
        }
        
        MPI_Barrier(MPI_COMM_WORLD);
        
        // 2. Ejecutar la Compresión Paralela
        RLECompressor::RunParallel(INPUT_FILE, OUTPUT_FILE, rank, size);

        // 3. Verificación Exacta Byte a Byte (Solo Rank 0)
        if (rank == 0) {
            cout << "\n--- Verificación Exacta Byte a Byte ---" << endl;
            
            // 3a. Generar el resultado comprimido IDEAL (secuencial)
            vector<uint8_t> expected_compressed = RLECompressor::Comprimir_Local(original_data);
            
            // 3b. Cargar el resultado producido por la versión paralela
            ifstream ifs(OUTPUT_FILE, ios::binary | ios::ate);
            if (!ifs) {
                cerr << "ERROR: No se pudo abrir el archivo de salida para verificación: " << OUTPUT_FILE << endl;
                remove(INPUT_FILE.c_str());
                return;
            }

            size_t actual_size = ifs.tellg();
            ifs.seekg(0, ios::beg);
            vector<uint8_t> actual_compressed(actual_size);
            ifs.read((char*)actual_compressed.data(), actual_size);
            ifs.close();

            // 3c. Comparar
            if (actual_compressed.size() == expected_compressed.size() &&
                memcmp(actual_compressed.data(), expected_compressed.data(), actual_size) == 0) {
                
                cout << "✅ ÉXITO: El resultado comprimido coincide byte a byte con la versión secuencial ideal." << endl;

            } else {
                cout << "❌ FALLO: El resultado comprimido NO coincide con la versión secuencial." << endl;
                cout << "  Tamaño esperado: " << expected_compressed.size() << " B, Real: " << actual_size << " B" << endl;
                
                // Imprimir para depuración
                cout << "  Esperado (Hex): "; for (uint8_t b : expected_compressed) printf("%02X ", b); cout << endl;
                cout << "  Real (Hex):     "; for (uint8_t b : actual_compressed) printf("%02X ", b); cout << endl;

                assert(false); // Forzar fallo
            }
            
            // Limpieza después de cada prueba
            remove(INPUT_FILE.c_str());
            remove(OUTPUT_FILE.c_str());
        }
        
        MPI_Barrier(MPI_COMM_WORLD);
    }
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
        cout << "\n--- INICIO DE PRUEBA DE FRONTERAS RLE (" << size << " Procesos) ---" << endl;
    }
    
    run_boundary_test(rank, size);

    MPI_Finalize();
    return 0;
}