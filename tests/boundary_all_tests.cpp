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
const string DECOMPRESSED_FILE = "test_data/boundary_decompressed.bin";

// --- Función para crear el archivo de prueba (Solo Rank 0) ---
vector<uint8_t> create_test_file(const string& file_name, const vector<uint8_t>& data) {
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
void run_full_test_cycle(int rank, int size, const vector<uint8_t>& original_data, const string& case_name) {
    if (rank == 0) {
        cout << "\n==========================================================" << endl;
        cout << "INICIANDO CASO DE PRUEBA: " << case_name << endl;
        cout << "==========================================================" << endl;
        create_test_file(INPUT_FILE, original_data);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    
    // ==========================================================
    // 1. COMPRESIÓN PARALELA Y VERIFICACIÓN DE FRONTERA
    // ==========================================================
    RLECompressor::RunParallel(INPUT_FILE, OUTPUT_FILE, rank, size);

    // 2. Verificación Exacta Byte a Byte de Compresión (Solo Rank 0)
    if (rank == 0) {
        cout << "\n--- Verificación Compresión (" << case_name << ") ---" << endl;
        
        vector<uint8_t> expected_compressed = RLECompressor::Comprimir_Local(original_data);
        
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

        if (actual_compressed.size() == expected_compressed.size() &&
            memcmp(actual_compressed.data(), expected_compressed.data(), actual_size) == 0) {
            
            cout << "Tamaño esperado: " << expected_compressed.size() << " B" << endl;
            cout << "Tamaño real:     " << actual_compressed.size() << " B" << endl;
            cout << "✅ ÉXITO: El resultado comprimido paralelo coincide byte a byte con la versión secuencial ideal." << endl;

        } else {
            cout << "❌ FALLO: El resultado comprimido no coincide con la versión secuencial." << endl;
            cout << "Tamaño esperado: " << expected_compressed.size() << " B, Real: " << actual_size << " B" << endl;
            
            // Imprimir para depuración
            cout << "  Esperado (Hex): "; for (uint8_t b : expected_compressed) printf("%02X ", b); cout << endl;
            cout << "  Real (Hex):     "; for (uint8_t b : actual_compressed) printf("%02X ", b); cout << endl;

            assert(false); // Forzar fallo
        }
    }
    
    MPI_Barrier(MPI_COMM_WORLD);

    // ==========================================================
    // 3. DESCOMPRESIÓN PARALELA Y VERIFICACIÓN FINAL
    // ==========================================================
    
    // Ejecutar la descompresión paralela
    RLECompressor::RunParallelDecompress(OUTPUT_FILE, DECOMPRESSED_FILE, rank, size);

    // 4. Verificación Exacta Byte a Byte del archivo descomprimido (Solo Rank 0)
    if (rank == 0) {
        cout << "\n--- Verificación Descompresión (" << case_name << ") ---" << endl;

        // Cargar el resultado descomprimido
        ifstream ifs_decomp(DECOMPRESSED_FILE, ios::binary | ios::ate);
        if (!ifs_decomp) {
            cerr << "ERROR: No se pudo abrir el archivo descomprimido para verificación: " << DECOMPRESSED_FILE << endl;
            remove(INPUT_FILE.c_str());
            remove(OUTPUT_FILE.c_str());
            return;
        }

        size_t decompressed_size = ifs_decomp.tellg();
        ifs_decomp.seekg(0, ios::beg);
        vector<uint8_t> actual_decompressed(decompressed_size);
        ifs_decomp.read((char*)actual_decompressed.data(), decompressed_size);
        ifs_decomp.close();

        // Comparar con los datos originales
        if (actual_decompressed.size() == original_data.size() &&
            memcmp(actual_decompressed.data(), original_data.data(), decompressed_size) == 0) {
            
            cout << "Tamaño original: " << original_data.size() << " B" << endl;
            cout << "Tamaño descomprimido: " << actual_decompressed.size() << " B" << endl;
            cout << "✅ ÉXITO: El archivo descomprimido coincide byte a byte con el original." << endl;

        } else {
            cout << "❌ FALLO: El resultado descomprimido NO coincide con el archivo original." << endl;
            cout << "Tamaño esperado: " << original_data.size() << " B, Real: " << decompressed_size << " B" << endl;
            
            // Imprimir para depuración
            cout << "  Original (Hex): "; for (uint8_t b : original_data) printf("%02X ", b); cout << endl;
            cout << "  Descomprimido (Hex): "; for (uint8_t b : actual_decompressed) printf("%02X ", b); cout << endl;

            assert(false); // Forzar fallo
        }

        // Limpieza de todos los archivos
        remove(INPUT_FILE.c_str());
        remove(OUTPUT_FILE.c_str());
        remove(DECOMPRESSED_FILE.c_str());
    }
}


int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2) {
        if (rank == 0) cout << "Se requieren al menos 2 procesos para la prueba de fronteras." << endl;
        MPI_Finalize();
        return 0;
    }

    if (rank == 0) {
        cout << "--- INICIO DE PRUEBA DE FRONTERAS RLE (" << size << " Procesos) ---" << endl;
    }
    
    // --- DEFINICIÓN DE CASOS DE PRUEBA ---
    
    // Caso 1: AAAABBB AA (9 bytes) - (r,4,A) | (r,3,B) (l,2,A) -> Boundary en medio del token B (Split por 5/4 bytes)
    // El fallo original ocurría aquí (P0: AAAAB, P1: BB AA)
    vector<uint8_t> case1_data = {
        65, 65, 65, 65, 66, 66, 66, 65, 65, // AAAA BBB AA
    }; 
    run_full_test_cycle(rank, size, case1_data, "Caso 1: AAAA-BBB-AA (Boundary en run B)");


    // Caso 2: AAAABBBB (8 bytes) - (r,4,A) | (r,4,B) -> Boundary EXACTAMENTE después del token A
    // P0: AAAA (4 bytes), P1: BBBB (4 bytes)
    // Fuerza a P1 a leer el FLAG [FF] del token A si el solapamiento es 2.
    vector<uint8_t> case2_data = {
        65, 65, 65, 65, 66, 66, 66, 66, // AAAA BBBB
    }; 
    run_full_test_cycle(rank, size, case2_data, "Caso 2: AAAA-BBBB (Boundary entre runs)");


    // Caso 3: C A B B B C (6 bytes) - l,1,C l,1,A (r,3,B) l,1,C -> Boundary en medio del token B (Split por 3/3 bytes)
    // P0: C A B, P1: B B C
    // Este caso asegura que P0 envíe el 'B' incompleto a P1 para fusión.
    vector<uint8_t> case3_data = {
        67, 65, 66, 66, 66, 67, // C A B B B C
    }; 
    run_full_test_cycle(rank, size, case3_data, "Caso 3: C-A-BBB-C (Boundary en run B con literales)");


    MPI_Finalize();
    return 0;
}