#include "../include/RLECompressor.hpp"
#include <iostream>
#include <vector>
#include <fstream>
#include <numeric>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <mpi.h>

using namespace std;

const string TEST_FILE = "test_data/mpi_io_temp.bin";
const size_t FILE_SIZE = 1000; 

void create_test_file(size_t size) {
    ofstream ofs(TEST_FILE, ios::binary);
    if (!ofs) {
        cerr << "Error al crear el archivo de prueba: " << TEST_FILE << endl;
        return;
    }
    vector<uint8_t> data(size);
    // Llenar con una secuencia predecible: 0, 1, 2, 3, 4, ...
    iota(data.begin(), data.end(), 0);
    ofs.write((const char*)data.data(), size);
    ofs.close();
}

/**
 * @brief Ejecuta la prueba de integración para el método Leer_Bloque_MPIIO.
 */
void run_mpi_io_test(int rank, int size) {
    size_t global_file_size = 0;
    size_t offset_start = 0;
    vector<uint8_t> buffer_in;

    RLECompressor::Leer_Bloque_MPIIO(TEST_FILE, rank, size, buffer_in, global_file_size, offset_start);
    
    size_t expected_base_size = FILE_SIZE / size;
    size_t remainder = FILE_SIZE % size;
    
    size_t expected_chunk_size = expected_base_size + ((size_t)rank < remainder ? 1 : 0);
    
    int expected_extra_byte = (rank < size - 1) ? 1 : 0;
    
    assert(buffer_in.size() == expected_chunk_size + expected_extra_byte); 
    size_t calculated_offset = (size_t)rank * expected_base_size + std::min((size_t)rank, remainder);
    assert(offset_start == calculated_offset);
    
    if (rank == 0) {
        cout << "P0: Particionamiento y offset verificados correctamente." << endl;
    }
    
    int local_read_size = buffer_in.size();
    vector<int> global_read_sizes(size);
    
    MPI_Gather(&local_read_size, 1, MPI_INT, global_read_sizes.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        vector<int> displacements(size, 0);
        size_t total_read_size = 0;
        
        for (int i = 0; i < size; ++i) {
            displacements[i] = total_read_size;
            total_read_size += global_read_sizes[i];
        }
        
        vector<uint8_t> global_read_buffer(total_read_size);
        
        MPI_Gatherv(buffer_in.data(), local_read_size, MPI_UNSIGNED_CHAR,
                    global_read_buffer.data(), global_read_sizes.data(), displacements.data(), 
                    MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

        bool size_ok = total_read_size == (FILE_SIZE + (size - 1));
        
        bool data_matches = true;
        for (int i = 0; i < size; ++i) {
            size_t start_index = displacements[i];
            size_t expected_value = (size_t)i * expected_base_size + std::min((size_t)i, remainder);
            
            if (global_read_buffer[start_index] != (uint8_t)(expected_value % 256)) {
                data_matches = false;
                break;
            }
        }

        if (size_ok && data_matches) {
            cout << "P0: PASÓ la Prueba de Integración MPI-IO. Datos leídos correctamente." << endl;
        } else {
            cout << "P0: FALLÓ la Prueba de Integración MPI-IO." << endl;
            if (!size_ok) cout << "P0: FALLO: El tamaño total leido (" << total_read_size << ") es incorrecto." << endl;
            if (!data_matches) cout << "P0: FALLO: Los datos en las fronteras no coinciden con lo esperado." << endl;
        }
        
    } else {
        MPI_Gatherv(buffer_in.data(), local_read_size, MPI_UNSIGNED_CHAR,
                    nullptr, nullptr, nullptr, 
                    MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);
    }
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
        cout << "\n--- INICIO DE PRUEBA DE INTEGRACIÓN MPI-IO (" << size << " Procesos) ---" << endl;
        create_test_file(FILE_SIZE);
    }
    
    MPI_Barrier(MPI_COMM_WORLD);
    
    try {
        run_mpi_io_test(rank, size);
    } catch (const std::exception& e) {
        cerr << "P" << rank << ": Excepción durante la prueba: " << e.what() << endl;
    }
    
    if (rank == 0) {
        remove(TEST_FILE.c_str());
    }

    MPI_Finalize();
    return 0;
}