#!/bin/bash
# ----------------------------------------------------------------------
# SCRIPT DE GENERACIÓN DE DATOS BINARIOS PARA PRUEBAS DE EFICIENCIA
# ----------------------------------------------------------------------

# Tamaño en bytes (Ej: 100 MB)
SIZE_MB=100
SIZE=$((SIZE_MB * 1024 * 1024))
OUTPUT_DIR="test_data"

mkdir -p $OUTPUT_DIR

echo "Generando archivos de prueba de $SIZE_MB MB..."

# 1. Archivo Plano (Muy Alta Repetitividad)
# Genera 100MB de un único byte (ASCII 'A' = 0x41)
echo "  -> Generando data_plana.bin"
python3 -c "import sys; sys.stdout.buffer.write(b'\x41' * $SIZE)" > "$OUTPUT_DIR/data_plana.bin"

# 2. Archivo Aleatorio (Muy Baja Repetitividad)
# Genera 100MB de bytes aleatorios (peor caso para RLE)
echo "  -> Generando data_aleatoria.bin"
head -c $SIZE /dev/urandom > "$OUTPUT_DIR/data_aleatoria.bin"

# 3. Archivo Tipo Malla (Media Repetitividad)
# Genera 100MB del patrón "0123456789"
PATTERN="0123456789"
echo "  -> Generando data_malla.bin"
python3 -c "import sys; sys.stdout.buffer.write(b'${PATTERN}' * int($SIZE / ${#PATTERN}))" > "$OUTPUT_DIR/data_malla.bin"

echo "Archivos generados en el directorio: $OUTPUT_DIR"
echo "Para calcular métricas, use make run ARGS=\"test_data/archivo.bin\""