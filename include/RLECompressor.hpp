/**
 * PROYECTO: Parallel-Project-RLE
 * @author Medina Peralta Joaquín
 * @license General Public License (GPL) - O cualquier otra licencia que uses.
 */
#ifndef RLE_COMPRESSOR_HPP
#define RLE_COMPRESSOR_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <mpi.h>

/**
 * @brief Clase que contiene la lógica de la compresión y descompresión RLE
*/
class RLECompressor {
public:
    /**
     * @brief Comprime un archivo RLE usando MPI (Paralelo).
     */
    static void RunParallel(const std::string& input_file, const std::string& output_file, int rank, int size);
    
    /**
     * @brief Comprime un archivo RLE de forma normal (Secuencial).
     */
    static void RunSequential(const std::string& input_file, const std::string& output_file);
    
    /**
     * @brief Descomprime un archivo RLE usando MPI (Paralelo).
     */
    static void RunParallelDecompress(const std::string& input_file, const std::string& output_file, int rank, int size);
    
    /**
     * @brief Descomprime un archivo RLE de forma normal (Secuencial).
     */
    static void RunSequentialDecompress(const std::string& input_file, const std::string& output_file);

    /**
     * @brief Realiza la compresión RLE en un bloque de datos local.
     */
    static std::vector<uint8_t> Comprimir_Local(const std::vector<uint8_t>& buffer);
    
    /**
     * @brief Realiza la descompresión RLE en un bloque de datos local.
     */
    static std::vector<uint8_t> Descomprimir_Local(const std::vector<uint8_t>& compressed_buffer);
    
    /**
     * @brief Corrige (recorta) los datos descomprimidos en las fronteras entre procesos.
     * (Se usa para eliminar la redundancia del solapamiento.)
     */
    static void Corregir_Fronteras(std::vector<uint8_t>& local_output, const std::vector<uint8_t>& buffer_leido, size_t chunk_size, int rank, int size);
    
    /**
     * @brief Lee el bloque de datos asignado a un proceso usando MPI-I/O.
     */
    static void Leer_Bloque_MPIIO(const std::string& input_file, int rank, int size, std::vector<uint8_t>& buffer_in, size_t& global_file_size,size_t& offset_start);
};

// Funciones de prueba/utilidad
extern std::vector<uint8_t> Comprimir_Local_Test(const std::vector<uint8_t>& buffer);
extern std::vector<uint8_t> Descomprimir_Local_Test(const std::vector<uint8_t>& compressed_buffer);

// Flags o marcadores usados en el protocolo RLE
// Marcador para secuencua repetida
extern const std::uint8_t FLAG_RLE;
// Marcador para byte literal simple
extern const std::uint8_t FLAG_LITERAL;

#endif