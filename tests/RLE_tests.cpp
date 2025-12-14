#include "../include/RLECompressor.hpp"
#include <iostream>
#include <vector>
#include <cassert>
#include <cstring>
#include <cstdint>

using namespace std;

// Declaraciones de funciones y constantes globales (para el linking)
extern vector<uint8_t> Comprimir_Local_Test(const vector<uint8_t>& buffer);
extern vector<uint8_t> Descomprimir_Local_Test(const vector<uint8_t>& compressed_buffer);
extern const uint8_t FLAG_RLE;
extern const uint8_t FLAG_LITERAL;

// Función de ayuda para comparar buffers binarios
bool compare_buffers(const vector<uint8_t>& a, const vector<uint8_t>& b) {
    return a.size() == b.size() && memcmp(a.data(), b.data(), a.size()) == 0;
}

void test_compresion_rle_extendido() {
    cout << "  - Ejecutando: Compresion RLE Extendido (AAAAB C DDDE)" << endl;
    
    // Secuencia: AAAA B C DDD E (ASCII: 65, 66, 67, 68, 69)
    vector<uint8_t> input = {65, 65, 65, 65, 66, 67, 68, 68, 68, 69}; 
    
    // Salida Esperada: [FLAG_RLE] [4] [65] | [66] | [67] | [FLAG_RLE] [3] [68] | [69]
    vector<uint8_t> expected = {
        FLAG_RLE, 4, 65, // AAAA (4 repeticiones)
        66, // B (Literal, conteo < 3)
        67, // C (Literal)
        FLAG_RLE, 3, 68, // DDD (3 repeticiones)
        69  // E (Literal)
    };
    
    vector<uint8_t> actual = Comprimir_Local_Test(input);
    
    assert(compare_buffers(actual, expected) && "Fallo: Compresion RLE Extendido incorrecta.");
    cout << "  - PASÓ: Compresion RLE Extendido" << endl;
}

void test_modo_literal_escape() {
    cout << "  - Ejecutando: Modo Literal Escape (FF, FE, A)" << endl;
    
    // Prueba de que los FLAGs son correctamente escapados.
    // Input: [0xFF] [0xFE] [0x41]
    vector<uint8_t> input = {FLAG_RLE, FLAG_LITERAL, 65}; 

    // Salida Esperada (Cada FLAG se escapa con [FLAG_LITERAL]):
    // [FLAG_LITERAL] [FLAG_RLE] | [FLAG_LITERAL] [FLAG_LITERAL] | [65]
    vector<uint8_t> expected = {
        FLAG_LITERAL, FLAG_RLE, 
        FLAG_LITERAL, FLAG_LITERAL, 
        65
    };
    
    vector<uint8_t> actual = Comprimir_Local_Test(input);
    
    assert(compare_buffers(actual, expected) && "Fallo: Escape literal incorrecto.");
    cout << "  - PASÓ: Modo Literal Escape" << endl;
}

void test_inversion_completa() {
    cout << "  - Ejecutando: Inversion Completa (Compresion y Descompresion)" << endl;
    
    // Secuencia: AAAA B CC DDD E FF
    vector<uint8_t> input = {65, 65, 65, 65, 66, 67, 67, 68, 68, 68, 69, 0xFF, 0xFF}; 
    
    vector<uint8_t> compressed = Comprimir_Local_Test(input);
    vector<uint8_t> decompressed = Descomprimir_Local_Test(compressed);
    
    assert(compare_buffers(decompressed, input) && "Fallo: La inversion de compresion/descompresion es incorrecta.");
    cout << "  - PASÓ: Inversion Completa" << endl;
}

int main(int argc, char* argv[]) {
    // MPI_Init/Finalize no son necesarios para pruebas unitarias de lógica local
    cout << "--- EJECUCIÓN DE PRUEBAS UNITARIAS DE RLE EXTENDIDO ---" << endl;
    
    test_compresion_rle_extendido();
    test_modo_literal_escape();
    test_inversion_completa();
    
    cout << "\n--- TODAS LAS PRUEBAS UNITARIAS DE RLE PASARON ---" << endl;
    return 0;
}