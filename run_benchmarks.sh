#!/bin/bash

EXECUTABLE="./build/rle_compressor"
DATA_DIR="test_data"
INPUT_FILES=("data_plana.bin" "data_aleatoria.bin" "data_malla.bin")
N_PROCESSES=(2 4 6 8 10 16)  
REPETITIONS=5               
OUTPUT_CSV="benchmark_results_$(date +%Y%m%d_%H%M%S).csv"

GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m' # No Color

if [ ! -f "$EXECUTABLE" ]; then
    echo -e "${RED}ERROR:${NC} Ejecutable '$EXECUTABLE' no encontrado. Por favor, compile y asegúrese de que esté en el directorio correcto (ejecute 'make all')."
    exit 1
fi

if [ ! -d "$DATA_DIR" ]; then
    echo -e "${RED}ERROR:${NC} Directorio de datos '$DATA_DIR' no encontrado. Ejecute 'make generate_data' primero."
    exit 1
fi

echo "archivo,modo,procesos,repeticion,tiempo_s,tamano_original_B,tamano_comprimido_B" > "$OUTPUT_CSV"
echo -e "${GREEN}Resultados se guardarán en: ${OUTPUT_CSV}${NC}"

run_test() {
    local file_path=$1
    local mode=$2
    local n_procs=$3
    local rep=$4

    local command
    local output_file="${file_path}.rle"
    local log_file="temp_log_$(basename $file_path)_${n_procs}_${rep}.txt"
    local mpirun_flags=""

    if [ "$mode" == "--secuencial" ]; then
        echo -e "${BLUE}  -> Ejecutando Secuencial (Rep $rep)...${NC}"
        command="$EXECUTABLE $file_path --secuencial --output $output_file"
        mpirun -np 1 $command > "$log_file" 2>&1
    else
        echo -e "${BLUE}  -> Ejecutando Paralelo con $n_procs P (Rep $rep)...${NC}"
        command="$EXECUTABLE $file_path --parallel --output $output_file"
        
        mpirun_flags="--oversubscribe --bind-to none"
        
        mpirun -np $n_procs $mpirun_flags $command > "$log_file" 2>&1
    fi
    
    local time=$(grep "Tiempo:" "$log_file" | awk '{print $2}')
    local size_orig=$(grep "Tamaño Original:" "$log_file" | awk '{print $3}')
    local size_comp=$(grep "Tamaño Comprimido:" "$log_file" | awk '{print $3}')
    local file_name=$(basename "$file_path")
    local proc_str="1" 
    if [ "$mode" == "--parallel" ]; then
        proc_str="$n_procs"
    fi

    if [ ! -z "$time" ]; then
        echo "$file_name,$(echo $mode | sed 's/--//'),$proc_str,$rep,$time,$size_orig,$size_comp" >> "$OUTPUT_CSV"
        echo -e "    ${GREEN}OK:${NC} T=$time s"
        rm -f "$log_file"
    else
        echo -e "    ${RED}FALLO:${NC} No se pudo extraer el tiempo de ejecución. Revisar: $log_file"
        echo "$file_name,$(echo $mode | sed 's/--//'),$proc_str,$rep,FALLO,FALLO,FALLO" >> "$OUTPUT_CSV"
    fi

    rm -f "$output_file"
}

for file in "${INPUT_FILES[@]}"; do
    FILE_PATH="$DATA_DIR/$file"
    echo ""
    echo -e "${GREEN}======================================================${NC}"
    echo -e "${GREEN}INICIANDO BENCHMARK PARA ARCHIVO: ${file}${NC}"
    echo -e "${GREEN}======================================================${NC}"

    echo ""
    echo -e "${BLUE}--- PRUEBAS SECUENCIALES (P=1) ---${NC}"
    for (( i=1; i<=$REPETITIONS; i++ )); do
        run_test "$FILE_PATH" "--secuencial" 1 $i
    done
    
    for procs in "${N_PROCESSES[@]}"; do
        echo ""
        echo -e "${BLUE}--- PRUEBAS PARALELAS (P=${procs}) ---${NC}"
        for (( i=1; i<=$REPETITIONS; i++ )); do
            run_test "$FILE_PATH" "--parallel" $procs $i
        done
    done
done

echo ""
echo -e "${GREEN}======================================================${NC}"
echo -e "${GREEN}BENCHMARK COMPLETO.${NC}"
echo -e "${GREEN}Resultados en: ${OUTPUT_CSV}${NC}"
echo "--------------------------------------------------------"

echo -e "${BLUE}--- RESULTADOS RESUMIDOS ---${NC}"

awk -F ',' '
    BEGIN {
        OFS="|";
        printf "\n"
        printf "%-20s | %-8s | %-18s | %-12s\n", "Archivo", "Procesos", "Tiempo Promedio (s)", "Speedup"
        printf "%s\n", "---------------------|----------|--------------------|--------------"
    }

    # Procesar datos
    NR == 1 || $5 == "FALLO" { next } # Saltar encabezado y fallos
    {
        key = $1 "," $3
        sum_time[key] += $5
        count[key]++
    }
    
    # Procesar resultados y calcular Speedup
    END {
        for (key in sum_time) {
            split(key, arr, ",")
            file = arr[1]
            procs = arr[2]
            
            if (procs == 1) {
                sequential_time[file] = sum_time[key] / count[key]
            }
        }
        
        asorti(sum_time, sorted_keys)

        for (i in sorted_keys) {
            key = sorted_keys[i]
            split(key, arr, ",")
            file = arr[1]
            procs = arr[2]

            avg_time = sum_time[key] / count[key]
            
            speedup = "N/A"
            if (sequential_time[file] > 0) {
                speedup = sequential_time[file] / avg_time
            }
            
            printf "%-20s | %-8s | %-18.4f | %-12.2f\n", file, procs, avg_time, (speedup == "N/A" ? 0 : speedup)
        }
    }' "$OUTPUT_CSV" 

echo "--------------------------------------------------------"
