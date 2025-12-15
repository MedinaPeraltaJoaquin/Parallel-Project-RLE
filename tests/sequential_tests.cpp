/**
 * PROYECTO: Parallel-Project-RLE
 * @author Medina Peralta Joaquín
 * @license General Public License (GPL) - O cualquier otra licencia que uses.
 */

#include "../include/RLECompressor.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdio>
#include <cassert>
#include <cstring>

using namespace std;

const string SEQ_IN_FILE = "test_data/seq_in.bin";
const string SEQ_OUT_FILE = "test_data/seq_out.rle";

// --- Función para crear datos de prueba (alta repetición) ---
vector<uint8_t> create_seq_test_data() {
    // 50 'A's (65) + 3 'B's (66) + 2 'C's (67) -> 55 bytes
    vector<uint8_t> data(55);
    fill(data.begin(), data.begin() + 50, 65);
    fill(data.begin() + 50, data.begin() + 53, 66);
    fill(data.begin() + 53, data.end(), 67);
    return data;
}

void run_sequential_test() {
    cout << "\n--- INICIO DE PRUEBA SECUENCIAL RLE ---" << endl;
    
    vector<uint8_t> original_data = create_seq_test_data();
    ofstream ofs_in(SEQ_IN_FILE, ios::binary);
    ofs_in.write((const char*)original_data.data(), original_data.size());
    ofs_in.close();

    RLECompressor::RunSequential(SEQ_IN_FILE, SEQ_OUT_FILE);

    // 3. Generar el resultado comprimido IDEAL esperado
    // Expected: [FF 50 41] [FE 42] [42] [42] [43] [43] -> 3 + 2 + 1 + 1 + 1 + 1 = 9 bytes? NO.
    // Expected: [FF] [50] [A] (3 bytes)
    //           [B] [B] [B] (3 bytes, < RLE_THRESHOLD=3)
    //           [C] [C] (2 bytes, < RLE_THRESHOLD=3)
    //           -> 3 + 3 + 2 = 8 bytes.
    // Expected: [FF 50 A] [FF 3 B] [C] [C] (2 bytes, C directo) -> 3 + 3 + 2 = 8 bytes.
    
    const vector<uint8_t> expected_compressed = {
        0xFF, 50, 65, // 50 A's
        0xFF, 0x03, 66, // 3 B's
        67, 67 // 2 C's literales directos
    };
    
    ifstream ifs_out(SEQ_OUT_FILE, ios::binary | ios::ate);
    size_t actual_size = ifs_out.tellg();
    ifs_out.seekg(0, ios::beg);
    vector<uint8_t> actual_compressed(actual_size);
    ifs_out.read((char*)actual_compressed.data(), actual_size);
    ifs_out.close();

    if (actual_compressed.size() == expected_compressed.size() &&
        memcmp(actual_compressed.data(), expected_compressed.data(), actual_size) == 0) {
        
        cout << "ÉXITO: La compresión secuencial coincide con el resultado esperado (" << actual_size << " B)." << endl;
    } else {
        cout << "FALLO: El resultado secuencial no coincide." << endl;
        assert(false);
    }

    remove(SEQ_IN_FILE.c_str());
    remove(SEQ_OUT_FILE.c_str());
}

int main() {
    run_sequential_test();
    return 0;
}