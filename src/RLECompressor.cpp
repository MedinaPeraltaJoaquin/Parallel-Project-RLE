#include "../include/RLECompressor.hpp"
#include "../include/Timer.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <cstring>
#include <numeric>

using namespace std;

// --- CONSTANTES DE CODIFICACIÓN (Globales para pruebas) ---
const uint8_t FLAG_RLE = 0xFF;      // Flag RLE: [0xFF] [CONTEO] [VALOR]
const uint8_t FLAG_LITERAL = 0xFE;  // Flag Escape: [0xFE] [BYTE_ESCAPADO]
const size_t RLE_THRESHOLD = 3;     // Umbral mínimo para usar la tupla RLE
const int FRONTERA_TAG = 100;       // Frontera real del último byte

vector<uint8_t> RLECompressor::Comprimir_Local(const vector<uint8_t>& buffer) {
    vector<uint8_t> salida; 
    if (buffer.empty()) return salida;

    size_t i = 0;
    while (i < buffer.size()) {
        uint8_t valor_actual = buffer[i];
        size_t j = i;
        
        // Contar la secuencia de repetición (máximo 255)
        while (j < buffer.size() && buffer[j] == valor_actual && (j - i) < 255) {
            j++;
        }
        size_t conteo = j - i;

        if (conteo >= RLE_THRESHOLD) {
            // MODO DE REPETICIÓN (RLE): [FLAG_RLE] [CONTEO] [VALOR]
            salida.push_back(FLAG_RLE); 
            salida.push_back((uint8_t)conteo); 
            salida.push_back(valor_actual); 
        } else {
            // MODO LITERAL (Escribir los bytes directamente)
            for (size_t k = 0; k < conteo; ++k) {
                uint8_t literal_byte = buffer[i + k];
                
                if (literal_byte == FLAG_RLE || literal_byte == FLAG_LITERAL) {
                    salida.push_back(FLAG_LITERAL);
                }
                salida.push_back(literal_byte); 
            }
        }

        i += conteo;
    }
    return salida;
}

vector<uint8_t> RLECompressor::Descomprimir_Local(const vector<uint8_t>& compressed_buffer) {
    vector<uint8_t> salida;
    size_t i = 0;

    while (i < compressed_buffer.size()) {
        uint8_t byte = compressed_buffer[i];

        if (byte == FLAG_RLE) {
            // MODO RLE: Esperar CONTEO y VALOR
            if (i + 2 >= compressed_buffer.size()) break; 
            
            uint8_t conteo = compressed_buffer[i + 1];
            uint8_t valor = compressed_buffer[i + 2];
            
            for (int k = 0; k < conteo; ++k) salida.push_back(valor);
            i += 3; // Flag + Conteo + Valor

        } else if (byte == FLAG_LITERAL) {
            // MODO ESCAPE LITERAL
            if (i + 1 >= compressed_buffer.size()) break;
            
            salida.push_back(compressed_buffer[i + 1]);
            i += 2;
        }
        else {
            // MODO LITERAL DIRECTO
            salida.push_back(byte);
            i += 1;
        }
    }
    return salida;
}

// Funciones auxiliares para pruebas
vector<uint8_t> Comprimir_Local_Test(const vector<uint8_t>& buffer) {
    return RLECompressor::Comprimir_Local(buffer);
}
vector<uint8_t> Descomprimir_Local_Test(const vector<uint8_t>& compressed_buffer) {
    return RLECompressor::Descomprimir_Local(compressed_buffer);
}


void RLECompressor::Leer_Bloque_MPIIO(
    const std::string& input_file, 
    int rank, 
    int size, 
    std::vector<uint8_t>& buffer_in, 
    size_t& global_file_size,
    size_t& offset_start
) {
    MPI_File fh;
    int error;

    error = MPI_File_open(MPI_COMM_WORLD, input_file.c_str(), MPI_MODE_RDONLY, MPI_INFO_NULL, &fh);
    if (error != MPI_SUCCESS) {
        std::cerr << "P" << rank << ": Error al abrir el archivo: " << input_file << std::endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Offset file_size_mpi;
    
    MPI_File_get_size(fh, &file_size_mpi);
    global_file_size = (size_t)file_size_mpi;

    size_t chunk_base_size = global_file_size / size;
    size_t remainder = global_file_size % size;
    
    size_t my_chunk_size = chunk_base_size;
    if ((size_t)rank < remainder) {
        my_chunk_size += 1;
    }

    offset_start = ((size_t)rank < remainder) 
                   ? ((size_t)rank * (chunk_base_size + 1)) 
                   : ((size_t)rank * chunk_base_size + remainder);

    int extra_byte_to_read = (rank < size - 1) ? 1 : 0; 
    size_t read_size = my_chunk_size + extra_byte_to_read;
    
    buffer_in.resize(read_size);
    
    MPI_File_read_at(fh, offset_start, buffer_in.data(), read_size, MPI_UNSIGNED_CHAR, MPI_STATUS_IGNORE);
    
    MPI_File_close(&fh);
}

void RLECompressor::Corregir_Fronteras(std::vector<uint8_t>& local_output, const std::vector<uint8_t>& buffer_leido, size_t chunk_size, int rank, int size) {
    if (size == 1) return;
    
    uint8_t my_last_byte = (chunk_size > 0) ? buffer_leido[chunk_size - 1] : 0;
    uint8_t last_byte_prev = 0; 
    uint8_t first_byte_mine = (chunk_size > 0) ? buffer_leido[0] : 0; 
    const int FUSION_TAG = FRONTERA_TAG + 1;

    if (rank < size - 1) { 
        MPI_Send(&my_last_byte, 1, MPI_UNSIGNED_CHAR, rank + 1, FRONTERA_TAG, MPI_COMM_WORLD);
    }
    if (rank > 0) { 
        MPI_Recv(&last_byte_prev, 1, MPI_UNSIGNED_CHAR, rank - 1, FRONTERA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    
    
    int fusion_length_to_prev = 0; 
    
    if (rank > 0 && last_byte_prev == first_byte_mine) {
        size_t k = 0;
        while (k < chunk_size && buffer_leido[k] == first_byte_mine && k < 255) {
             k++;
        }
        fusion_length_to_prev = (int)k; 
    }
    
    
    int fusion_length_from_next = 0;

    if (rank > 0) { 
        MPI_Send(&fusion_length_to_prev, 1, MPI_INT, rank - 1, FUSION_TAG, MPI_COMM_WORLD);
    }
    if (rank < size - 1) { 
        MPI_Recv(&fusion_length_from_next, 1, MPI_INT, rank + 1, FUSION_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    
    
    if (fusion_length_from_next > 0 && !local_output.empty()) {
        
        if (local_output.size() >= 3 && local_output[local_output.size() - 3] == FLAG_RLE) {
            
            uint16_t new_count = (uint16_t)local_output[local_output.size() - 2] + fusion_length_from_next;
            
            if (new_count <= 255) {
                 local_output[local_output.size() - 2] = (uint8_t)new_count;
            } else {
                 local_output[local_output.size() - 2] = 255;
            }
            
        } 
        else { 
            if (local_output.back() == my_last_byte) {
                
                uint8_t literal_char = local_output.back();
                int literal_len = 0; 
                
                size_t i = local_output.size();
                while (i > 0) {
                    uint8_t code = local_output[i - 1];
                    
                    if (code == literal_char) {
                        literal_len++;
                        i--;
                    } else if (code == FLAG_LITERAL && i > 1 && local_output[i - 2] != FLAG_RLE) {
                         if (local_output[i-1] == literal_char) {
                             literal_len++;
                             i -= 2;
                         } else {
                             break;
                         }
                    } else {
                        break;
                    }
                }

                local_output.erase(local_output.begin() + i, local_output.end());
                
                uint16_t new_count = (uint16_t)literal_len + fusion_length_from_next;
                
                if (new_count >= RLE_THRESHOLD) {
                    local_output.push_back(FLAG_RLE);
                    local_output.push_back((uint8_t)new_count);
                    local_output.push_back(literal_char);
                } else {
                    for (int n = 0; n < new_count; ++n) {
                         if (literal_char == FLAG_RLE || literal_char == FLAG_LITERAL) {
                            local_output.push_back(FLAG_LITERAL);
                         }
                         local_output.push_back(literal_char);
                    }
                }
            }
        }
    }
    
    if (fusion_length_to_prev > 0) {
        
        if (local_output.empty()) return;

        int absorbed_length = 0;
        
        while (absorbed_length < fusion_length_to_prev && !local_output.empty()) {
            uint8_t first_code = local_output[0];
            
            if (first_code == FLAG_RLE) {
                absorbed_length += local_output[1];
                local_output.erase(local_output.begin(), local_output.begin() + 3);
                
            } else if (first_code == FLAG_LITERAL) {
                absorbed_length += 1;
                local_output.erase(local_output.begin(), local_output.begin() + 2);
                
            } else {
                absorbed_length += 1;
                local_output.erase(local_output.begin());
            }
        }
    }
}

void RLECompressor::RunParallel(const std::string& input_file, const std::string& output_file, int rank, int size) {
    Timer t;
    size_t global_file_size = 0;
    size_t offset_start = 0;
    vector<uint8_t> buffer_in;
    
    Leer_Bloque_MPIIO(input_file, rank, size, buffer_in, global_file_size, offset_start);
    
    size_t chunk_size = buffer_in.size();
    if (rank < size - 1) {
        chunk_size--;
    }

    vector<uint8_t> data_to_compress(buffer_in.begin(), buffer_in.begin() + chunk_size);
    vector<uint8_t> local_compressed_output = Comprimir_Local(data_to_compress);

    Corregir_Fronteras(local_compressed_output, buffer_in, chunk_size, rank, size);
    
    int local_len = local_compressed_output.size();
    vector<int> global_lengths(size);
    
    MPI_Gather(&local_len, 1, MPI_INT, global_lengths.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);

    vector<int> displacements(size, 0);
    size_t total_compressed_size = 0;
    vector<uint8_t> global_compressed_output;

    if (rank == 0) {
        for (int i = 0; i < size; ++i) {
            displacements[i] = (i > 0) ? (displacements[i-1] + global_lengths[i-1]) : 0;
            total_compressed_size += global_lengths[i];
        }
        global_compressed_output.resize(total_compressed_size);
    }
    
    MPI_Gatherv(
        local_compressed_output.data(), local_len, MPI_UNSIGNED_CHAR,
        global_compressed_output.data(), global_lengths.data(), displacements.data(), 
        MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD
    );

    if (rank == 0) {
        double elapsed = t.stop();
        cout << "--- Resultado de Compresión Paralela (" << size << " P) ---" << endl;
        cout << "Tiempo: " << fixed << setprecision(4) << elapsed << " s" << endl;
        cout << "Tamaño Original: " << global_file_size << " B" << endl;
        cout << "Tamaño Comprimido: " << total_compressed_size << " B" << endl;

        ofstream ofs(output_file, ios::binary);
        if (ofs.is_open()) {
            ofs.write((const char*)global_compressed_output.data(), total_compressed_size);
            ofs.close();
        } else {
            cerr << "P0: ERROR al abrir el archivo de salida para escritura: " << output_file << endl;
        }
    }
}

void RLECompressor::RunSequential(const std::string& input_file, const std::string& output_file) {
    Timer t;
    ifstream is(input_file, ios::binary | ios::ate);
    if (!is.is_open()) {
        cerr << "ERROR: No se pudo abrir el archivo de entrada: " << input_file << endl;
        return;
    }

    size_t size = is.tellg();
    is.seekg(0, ios::beg);

    vector<uint8_t> buffer(size);
    is.read((char*)buffer.data(), size);
    is.close();

    vector<uint8_t> compressed = Comprimir_Local(buffer);

    double elapsed = t.stop();
    cout << "--- Resultado de Compresión Secuencial (T1) ---" << endl;
    cout << "Tiempo: " << fixed << setprecision(4) << elapsed << " s" << endl;
    cout << "Tamaño Original: " << size << " B" << endl;
    cout << "Tamaño Comprimido: " << compressed.size() << " B" << endl;

    ofstream ofs(output_file, ios::binary);
    if (ofs.is_open()) {
        ofs.write((const char*)compressed.data(), compressed.size());
        ofs.close();
    } else {
        cerr << "ERROR: No se pudo abrir el archivo de salida para escritura: " << output_file << endl;
    }
}