#!/bin/bash

# Array of executables to test
executables=("ver_10" "ver_9" "ver_8" "ver_7" "ver_6")
# executables=("ver_10")

# Compile all versions first
echo "Compiling benchmark executables..."
for exe in "${executables[@]}"; do
    echo "Compiling $exe from ${exe}.c..."
    if [ "$exe" == "ver_6" ]; then
        gcc -O2 -o "$exe" "${exe}.c" -lgmp -lpthread -lm
    else
        # For ver_7, ver_8, ver_9, link with hybrid_mul.o
        gcc -O2 -o "$exe" "${exe}.c" hybrid_mul.o -lgmp -lpthread -lm
    fi

    if [ $? -ne 0 ]; then
        echo "Error: Compilation of $exe failed. Aborting." >&2
        exit 1
    fi
done
echo "Compilation complete."
echo "" # Add a blank line for readability

# Input files
# input_files=(
#     "../input/input-100k.txt"
#     # "../input/input-500k.txt"
#     "../input/input-1000k.txt"
#     # "../input/input-1500k.txt"
#     # "../input/input-2000k.txt"
#     # "../input/input-2500k.txt"
#     # "../input/input-3000k.txt"
#     # "../input/input-3500k.txt"
#     "../input/input-4100k.txt"
#     # "../input/sorted_input-4100k.txt"
#     # "../input/input-4500k.txt"
#     # "../input/input-5000k.txt"
#     # "../input/input-5500k.txt"
#     # "../input/input-6000k.txt"
#     # "../input/input-6500k.txt"
#     # "../input/input-7000k.txt"
#     "../input/moduli_valid.txt"
#     "../input/max_sorted.txt"
#     # "../input/max_moduli.txt"
# )


input_files=(
    "../input/input-1000k.txt"
)

# Temporary directory for output files
temp_dir=$(mktemp -d)

# --- Outer Loop: Process each input file ---
for input_file in "${input_files[@]}"; do
    echo "=================================================="
    echo "Processing input file: $input_file"
    
    # Generate output CSV name based on input filename
    input_basename=$(basename "$input_file" .txt)
    output_csv="comparison_results_${input_basename}.csv"
    
    # --- Stage 1: Initialize for current input ---
    echo "Initializing benchmark for $input_file..."
    if [ -f "$output_csv" ]; then
        echo "Removing existing $output_csv..."
        rm "$output_csv"
    fi

    # Header written flag for this input file
    header_written=false
    max_product_levels=0
    max_remainder_levels=0

    # --- Stage 2: Run and Parse incrementally ---
    echo "Running executables and capturing output..."

    for exe in "${executables[@]}"; do
        echo "--------------------------------------------------"
        echo "Processing $exe with $input_file..."
        log_file="$temp_dir/${exe}_${input_basename}.log"
        
        # Run executable
        ./"$exe" "$input_file" > "$log_file" 2>&1
        
        if [ ! -s "$log_file" ]; then
            echo "Error: $exe produced no output or failed to run."
            continue
        fi

        # Parse levels for header generation (only if header not written)
        # We assume the first successful run determines the structure
        if [ "$header_written" = false ]; then
            num_product_levels=$(grep "Product tree level" "$log_file" | wc -l)
            num_remainder_levels=$(grep "Remainder tree level" "$log_file" | wc -l)
            
            max_product_levels=$num_product_levels
            max_remainder_levels=$num_remainder_levels
            
            # Construct and write CSV header
            header="Version,Product_Tree_Total,Remainder_Tree_Total,Weak_Key_Count,Weak_Key_Time,Total_Time"
            
            for ((i=1; i<=max_product_levels; i++)); do
                header+=',Product_Lvl_'$i
            done
            
            for ((i=1; i<=max_remainder_levels; i++)); do
                header+=',Remainder_Lvl_'$i
            done
            
            echo "$header" > "$output_csv"
            header_written=true
            echo "Header written to $output_csv"
        fi

        # Parse results
        product_time="N/A"
        remainder_time="N/A"
        weak_key_count="N/A"
        weak_key_time="N/A"
        total_time="N/A"
        
        product_level_times=()
        remainder_level_times=()

        # Parse main stats
        product_time=$(grep "Product Tree completed at:" "$log_file" | awk '{print $6}')
        remainder_time=$(grep "Remainder Tree completed at:" "$log_file" | awk '{print $10}')
        weak_key_line=$(grep "Weak keys identified at:" "$log_file")
        weak_key_count=$(echo "$weak_key_line" | awk '{print $2}')
        total_time=$(echo "$weak_key_line" | awk '{print $7}')
        weak_key_time=$(echo "$weak_key_line" | awk '{print $11}')

        # Set to N/A if parsing failed
        if [ -z "$product_time" ]; then product_time="N/A"; fi
        if [ -z "$remainder_time" ]; then remainder_time="N/A"; fi
        if [ -z "$weak_key_count" ]; then weak_key_count="N/A"; fi
        if [ -z "$weak_key_time" ]; then weak_key_time="N/A"; fi
        if [ -z "$total_time" ]; then total_time="N/A"; fi
        
        # Parse product level times
        mapfile -t product_level_times < <(grep "Product tree level" "$log_file" | awk '{print $8}')
        
        # Parse remainder level times
        mapfile -t remainder_level_times < <(grep "Remainder tree level" "$log_file" | awk '{print $8}')

        # Construct data row
        row="$exe,$product_time,$remainder_time,$weak_key_count,$weak_key_time,$total_time"

        # Add product level times to row
        for ((i=0; i<max_product_levels; i++)); do
            if [ -n "${product_level_times[i]}" ]; then
                row+=",${product_level_times[i]}"
            else
                row+=",N/A"
            fi
        done

        # Add remainder level times to row
        for ((i=0; i<max_remainder_levels; i++)); do
            if [ -n "${remainder_level_times[i]}" ]; then
                row+=",${remainder_level_times[i]}"
            else
                row+=",N/A"
            fi
        done

        # Append row to CSV
        echo "$row" >> "$output_csv"
        echo "Results for $exe appended to $output_csv"
    done
    echo "Completed processing for $input_file. Results in $output_csv"
done

# --- Cleanup ---
rm -rf "$temp_dir"

echo -e "\nBenchmark complete."
echo "Note: 'N/A' indicates a value could not be parsed, possibly due to an error during execution."
