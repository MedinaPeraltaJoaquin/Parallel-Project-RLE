CXX = mpicxx
RM = rm -f

# Directorios
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
TEST_DIR = tests

# Banderas de compilación
CXXFLAGS = -O3 -Wall -std=c++17 -I$(INC_DIR) -g

# Nombres de archivos
TARGET = rle_parallel
TEST_SRC = $(TEST_DIR)/RLE_tests.cpp
TEST_TARGET = $(BUILD_DIR)/rle_tests

TEST_MPI_SRC = $(TEST_DIR)/mpi_io_tests.cpp
TEST_MPI_TARGET = $(BUILD_DIR)/mpi_io_tests

TEST_BND_SRC = $(TEST_DIR)/boundary_tests.cpp
TEST_BND_TARGET = $(BUILD_DIR)/boundary_tests

TEST_SEQ_SRC = $(TEST_DIR)/sequential_tests.cpp
TEST_SEQ_TARGET = $(BUILD_DIR)/sequential_tests

# Archivos fuente y objeto
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

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

# Compilación del ejecutable de pruebas
$(TEST_TARGET): $(BUILD_DIR)/RLECompressor.o $(BUILD_DIR)/RLE_tests.o
	@echo "Enlazando unit tests..."
	$(CXX) $(BUILD_DIR)/RLECompressor.o $(BUILD_DIR)/RLE_tests.o -o $@

$(BUILD_DIR)/RLE_tests.o: $(TEST_SRC)
	@echo "Compilando unit test file..."
	$(CXX) $(CXXFLAGS) -c $< -o $@


# ==============================================================================
# REGLAS DE PRUEBAS Y GENERACIÓN DE DATOS (Añadir test_mpi_io)
# ==============================================================================

# Compilación del archivo objeto del test MPI
$(BUILD_DIR)/mpi_io_tests.o: $(TEST_MPI_SRC)
	@echo "Compilando test MPI-IO..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Enlace del ejecutable de prueba MPI
$(TEST_MPI_TARGET): $(BUILD_DIR)/RLECompressor.o $(BUILD_DIR)/mpi_io_tests.o
	@echo "Enlazando test MPI-IO..."
	$(CXX) $^ -o $@

# Compilación del archivo objeto del test de Fronteras
$(BUILD_DIR)/boundary_tests.o: $(TEST_BND_SRC)
	@echo "Compilando test de Fronteras RLE..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Enlace del ejecutable de prueba de Fronteras
$(TEST_BND_TARGET): $(BUILD_DIR)/RLECompressor.o $(BUILD_DIR)/boundary_tests.o
	@echo "Enlazando test de Fronteras..."
	$(CXX) $^ -o $@
# Compilación del archivo objeto del test secuencial
$(BUILD_DIR)/sequential_tests.o: $(TEST_SEQ_SRC)
	@echo "Compilando test Secuencial RLE..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Enlace del ejecutable de prueba secuencial
$(TEST_SEQ_TARGET): $(BUILD_DIR)/RLECompressor.o $(BUILD_DIR)/sequential_tests.o
	@echo "Enlazando test Secuencial..."
	$(CXX) $^ -o $@

# Objetivo 'test_sequential'
test_sequential: setup $(TEST_SEQ_TARGET)
	@echo "--------------------------------------------------------"
	@echo "Ejecutando prueba Secuencial RLE."
	./$(TEST_SEQ_TARGET)

# Nuevo objetivo 'test_boundary'
test_boundary: setup $(TEST_BND_TARGET)
	@echo "--------------------------------------------------------"
	@echo "Ejecutando prueba de Fronteras RLE con 2 procesos (mínimo)."
	mpirun -np 2 $(TEST_BND_TARGET)

# Nuevo objetivo 'test_mpi_io'
test_mpi_io: setup $(TEST_MPI_TARGET)
	@echo "--------------------------------------------------------"
	@echo "Ejecutando prueba de I/O distribuida con 4 procesos..."
	mpirun -np 4 $(TEST_MPI_TARGET)

test: setup $(TEST_TARGET)
	@echo "--------------------------------------------------------"
	@echo "Ejecutando pruebas de corrección..."
	./$(TEST_TARGET)

# Regla para generar datos de prueba
generate_data: setup
	@echo "Generando datos de prueba (100MB por archivo)..."
	chmod +x generate_test_data.sh
	./generate_test_data.sh


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
	@rm -r test_data
	@rm -f *.rle