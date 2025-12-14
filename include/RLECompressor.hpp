#ifndef RLE_COMPRESSOR_HPP
#define RLE_COMPRESSOR_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <mpi.h>

class RLECompressor {
public:
    // Puntos de entrada
    static void RunParallel(const std::string& input_file, const std::string& output_file, int rank, int size);
    static void RunSequential(const std::string& input_file, const std::string& output_file);
    // Funciones esenciales de RLE (implementadas con Modo Literal/RLE Extendido)
    static std::vector<uint8_t> Comprimir_Local(const std::vector<uint8_t>& buffer);
    static std::vector<uint8_t> Descomprimir_Local(const std::vector<uint8_t>& compressed_buffer);
    // Funciones de MPI
    static void Corregir_Fronteras(std::vector<uint8_t>& local_output, int rank, int size, std::vector<uint8_t>& byte_de_frontera_leido);
    static void Leer_Bloque_MPIIO(const std::string& input_file, int rank, int size, std::vector<uint8_t>& buffer_in, size_t& global_file_size,size_t& offset_start);
};

// Declaración de las funciones auxiliares para pruebas (necesario para el linking de RLE_tests.cpp)
extern std::vector<uint8_t> Comprimir_Local_Test(const std::vector<uint8_t>& buffer);
extern std::vector<uint8_t> Descomprimir_Local_Test(const std::vector<uint8_t>& compressed_buffer);
extern const std::uint8_t FLAG_RLE;
extern const std::uint8_t FLAG_LITERAL;

#endif // RLE_COMPRESSOR_HPP