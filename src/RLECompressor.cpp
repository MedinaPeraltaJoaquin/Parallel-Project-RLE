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

void RLECompressor::Leer_Bloque_MPIIO(const std::string& input_file, int rank, int size, std::vector<uint8_t>& buffer_in, size_t& global_file_size) {
    
}

void RLECompressor::Corregir_Fronteras(std::vector<uint8_t>& local_output, int rank, int size, std::vector<uint8_t>& byte_de_frontera_leido) {

}

void RLECompressor::RunParallel(const std::string& input_file, const std::string& output_file, int rank, int size) {
    
}


void RLECompressor::RunSequential(const std::string& input_file, const std::string& output_file) {
    
}