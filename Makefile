# ==============================================================================
# CONFIGURACIÓN GENERAL PARA COMPATIBILIDAD LINUX / MACOS
# ==============================================================================

CXX = mpicxx
RM = rm -f

# Directorios
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
TEST_DIR = tests

# Banderas de compilación
CXXFLAGS = -O3 -Wall -std=c++17 -I$(INC_DIR)

# Nombres de archivos
TARGET = rle_parallel
TEST_SRC = $(TEST_DIR)/RLE_tests.cpp
TEST_TARGET = $(BUILD_DIR)/rle_tests

# Archivos fuente y objeto
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

# ==============================================================================
# REGLAS PRINCIPALES
# ==============================================================================

.PHONY: all setup clean run test generate_data

all: setup $(BUILD_DIR)/$(TARGET)

setup:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p test_data

# Compilación del ejecutable principal
$(BUILD_DIR)/$(TARGET): $(OBJECTS)
	@echo "Enlazando $(TARGET)..."
	$(CXX) $(OBJECTS) -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "Compilando $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ==============================================================================
# REGLAS DE PRUEBAS Y GENERACIÓN DE DATOS
# ==============================================================================

# Compilación del ejecutable de pruebas
$(TEST_TARGET): $(BUILD_DIR)/RLECompressor.o $(BUILD_DIR)/RLE_tests.o
	@echo "Enlazando unit tests..."
	$(CXX) $(BUILD_DIR)/RLECompressor.o $(BUILD_DIR)/RLE_tests.o -o $@

$(BUILD_DIR)/RLE_tests.o: $(TEST_SRC)
	@echo "Compilando unit test file..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: setup $(TEST_TARGET)
	@echo "--------------------------------------------------------"
	@echo "Ejecutando pruebas de corrección..."
	./$(TEST_TARGET)

# Regla para generar datos de prueba
generate_data: setup
	@echo "Generando datos de prueba (100MB por archivo)..."
	./generate_test_data.sh # Asume que el script .sh está en la raíz

# ==============================================================================
# REGLAS DE EJECUCIÓN Y LIMPIEZA
# ==============================================================================

# Variables de ejecución con valores por defecto
NP ?= 1
ARGS ?= 

run: all
	@echo "--------------------------------------------------------"
	@echo "Ejecutando $(TARGET) con $(NP) procesos..."
	mpirun -np $(NP) $(BUILD_DIR)/$(TARGET) $(ARGS)

clean:
	@echo "Limpiando archivos compilados y de salida..."
	@rm -rf $(BUILD_DIR)
	@$(RM) *.rle