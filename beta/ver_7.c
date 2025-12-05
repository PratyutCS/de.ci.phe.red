#include <stdio.h>

#include <gmp.h>

#include <stdlib.h>

#include <math.h>

#include <time.h>

#include <pthread.h>

#include "hybrid_mul.h"

#define thread_num 40

mpz_t * readFile(char * fileName, int * size) {
    //printf("[READFILE] inputfileName is: %s\n", fileName);
    FILE * inputFile = fopen(fileName, "r");

    if (!inputFile) {
        perror("Error opening file");
        return NULL;
    }

    char * line = NULL;
    size_t line_length = 0;
    size_t line_count = 0;

    while (getline( & line, & line_length, inputFile) != -1) {
        line_count++;
    }

    //printf("[READFILE] line count is: %zd\n", line_count);
    mpz_t * array = (mpz_t * ) malloc(line_count * sizeof(mpz_t));
    rewind(inputFile);

    for (size_t i = 0; i < line_count; i++) {
        mpz_init(array[i]);
        if (getline( & line, & line_length, inputFile) == -1) {
            perror("Error reading line");
            fclose(inputFile);
            free(line);
            for (size_t j = 0; j < i; j++) {
                mpz_clear(array[j]);
            }
            free(array);
            return NULL;
        }
        mpz_set_str(array[i], line, 16);
    }

    fclose(inputFile);
    free(line);
    * size = line_count;
    return array;
}

typedef struct {
    mpz_t * current_row;
    mpz_t * prev_row;
    int start_idx;
    int end_idx;
}
ThreadData;

void * multiply(void * arg) {
    ThreadData * data = (ThreadData * ) arg;
    for (int i = data -> start_idx; i < data -> end_idx; i++) {
        mpz_mul(data -> current_row[i], data -> prev_row[2 * i], data -> prev_row[2 * i + 1]);
    }
    return NULL;
}

void * mod(void * arg) {
    // printf("Hello from thread\n");
    mpz_t sq1;
    mpz_init(sq1);
    ThreadData * data = (ThreadData * ) arg;
    for(int i = data -> start_idx; i < data -> end_idx; i++){
        mpz_mul(sq1, data -> current_row[i], data -> current_row[i]);
        mpz_mod(data -> current_row[i], data -> prev_row[i/2], sq1);
    }
    mpz_clear(sq1);
    // printf("starting index is: %d - ending index is: %d - diff is: %d\n", data -> start_idx, data -> end_idx, data -> end_idx - data -> start_idx);
    return NULL;
}

typedef struct {
    mpz_t * current_row;
    mpz_t * prev_row;
    mpz_t * weak_keys;
    int start_idx;
    int end_idx;
    int weak_key_count;
}
ThreadData1;

void * weak_key(void * args){
    mpz_t one;
    mpz_init(one);
    mpz_set_str(one, "1", 16);

    mpz_t res;
    mpz_init(res);

    ThreadData1 * data = (ThreadData1 * ) args;
    for(int i = data -> start_idx ; i < data -> end_idx; i++){
        mpz_div(res, data -> current_row[i], data -> prev_row[i]);
        mpz_gcd(res, res, data -> prev_row[i]);
        if(mpz_cmp(res, one) != 0){
            mpz_set( data -> weak_keys[i], data -> prev_row[i]);
            data -> weak_key_count += 1;
        }
    }
    mpz_clears(one, res, NULL);
    return NULL;
}

// MAIN --------------------------------------------------------------------------------------------------------------------------------------------------------

int main(int argc, char *argv[]) {
    struct timespec start, end1, end2, end3, level_start, level_end;
    //printf("[main] io works\n");
    int line_size;
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 1;
    }
    mpz_t * fileData = readFile(argv[1], & line_size);

    if (!fileData) {
        return 1;
    }
    clock_gettime(CLOCK_REALTIME, &start);

    int row = 0;
    for (double i = line_size; i >= 1.0; i = ceil(i / 2.0)) {
        row++;
        if (i == 1.0)
            break;
    }
    //printf("[MAIN] rows required is: %d\n", row);

    long long row_size[row];

    mpz_t ** array_of_arrays = (mpz_t ** ) malloc(row * sizeof(mpz_t * ));

    for (int i = line_size, count = 0, prev = 0; i >= 1; i = ceil(i / 2.0), count += 1) {
        clock_gettime(CLOCK_REALTIME, &level_start);
        array_of_arrays[count] = (mpz_t * ) malloc(i * sizeof(mpz_t));
        // printf("count is : %d i allocated is: %d prev is: %d\n", count, i, prev);

        for (int j = 0; j < i; j++) {
            mpz_init(array_of_arrays[count][j]);
        }
        // ASSIGNMENT
        if (i == line_size) {
            for (int j = 0; j < i; j++) {
                mpz_set(array_of_arrays[count][j], fileData[j]);
            }

        } else if (i == 1) {
            // Using hardcoded max_depth for simplicity. A more robust solution would calculate this dynamically.
            int max_depth_val = 3; // Based on previous hybrid_mul execution analysis
            hybrid_mul(array_of_arrays[count][0], array_of_arrays[count - 1][0], array_of_arrays[count - 1][1], 0, max_depth_val);
            row_size[count] = i;
            clock_gettime(CLOCK_REALTIME, &level_end);
            printf("[MAIN] Product tree level %d/%d calculated in %f seconds\n", count + 1, row, (level_end.tv_sec - level_start.tv_sec) + (level_end.tv_nsec - level_start.tv_nsec) / 1.0e9);
            break;
        } else {

            // THIS LOOP NEEDS MULTI PROCESSING 1

            pthread_t threads[thread_num];
            ThreadData threadData[thread_num];

            int q = floor(prev / 2);
            if (q != i) {
                // printf("q is: %d - prev is: %d - count is: %d - count-1 is: %d\n", q, prev - 1, count, count - 1);
                mpz_set(array_of_arrays[count][q], array_of_arrays[count - 1][prev - 1]);
            }

            long rem = q % thread_num;
            long load = q / thread_num;
            long fin_load = load;
            int j = 0;
            int th = 0;

            for (th = 0; th < thread_num; th++) {
                if (rem > 0) {
                    fin_load = load + 1;
                    rem -= 1;
                }
                else{
                    fin_load = load;
                }
                if (fin_load == 0) {
                    break;
                }
                threadData[th].current_row = array_of_arrays[count];
                threadData[th].prev_row = array_of_arrays[count - 1];
                threadData[th].start_idx = j;
                threadData[th].end_idx = j + fin_load;
                pthread_create( & threads[th], NULL, multiply, & threadData[th]);
                j += fin_load;
            }

            for (int p = 0; p < th; p++) {
                pthread_join(threads[p], NULL);
            }

            // for (int j = 0; j < i; j++) {
            //     if (mpz_cmp_ui(array_of_arrays[count][j], 0) == 0) {
            //         printf("0 found at index: %d of count; %d\n", j, count);
            //         exit(1);
            //     }
            // }
        }
        clock_gettime(CLOCK_REALTIME, &level_end);
        printf("[MAIN] Product tree level %d/%d calculated in %f seconds\n", count + 1, row, (level_end.tv_sec - level_start.tv_sec) + (level_end.tv_nsec - level_start.tv_nsec) / 1.0e9);
        prev = i;
        row_size[count] = i;
    }
    clock_gettime(CLOCK_REALTIME, &end1);
    printf("[MAIN] Product Tree completed at: %f seconds (elapsed)\n", (end1.tv_sec - start.tv_sec) + (end1.tv_nsec - start.tv_nsec) / 1.0e9);

    // mpz_out_str(stdout, 16, array_of_arrays[row-1][0]);
    // printf("\n");

    // for (int i = 0; i < row; i++) {
    //   printf("row is: %d its value is: %lld\n", i, row_size[i]);
    // }

    for (int i = row - 2; i >= 0; i--) {
        clock_gettime(CLOCK_REALTIME, &level_start);
        // printf("row size for %d is : %lld\n", i, row_size[i]);

        pthread_t threads[thread_num];
        ThreadData threadData[thread_num];

        int rem = row_size[i] % thread_num;
        int load = row_size[i] / thread_num;
        int fin_load = 0;
        int j = 0;
        int th = 0;
        // printf("rem is : %d -  load is: %d\n", rem, load);

        for(th = 0 ; th < thread_num ; th++){
            if(rem > 0){
                // printf("rem > 0 - rem is: %d - fin_load is: %d\n", rem, fin_load);
                fin_load = load + 1;
                rem -= 1;
            }
            else{
                fin_load = load;
            }
            if(fin_load == 0){
                break;
            }
            threadData[th].current_row = array_of_arrays[i];
            threadData[th].prev_row = array_of_arrays[i+1];
            threadData[th].start_idx = j;
            threadData[th].end_idx = j+fin_load;
            pthread_create( & threads[th], NULL, mod, & threadData[th]);
            j+= fin_load;

        }
        for (int p = 0; p < th; p++) {
            pthread_join(threads[p], NULL);
        }
        clock_gettime(CLOCK_REALTIME, &level_end);
        printf("[MAIN] Remainder tree level %d/%d calculated in %f seconds\n", row - 1 - i, row - 1, (level_end.tv_sec - level_start.tv_sec) + (level_end.tv_nsec - level_start.tv_nsec) / 1.0e9);
    }
    clock_gettime(CLOCK_REALTIME, &end2);
    printf("[MAIN] Remainder Tree completed at: %f seconds (elapsed), duration: %f seconds\n", (end2.tv_sec - start.tv_sec) + (end2.tv_nsec - start.tv_nsec) / 1.0e9, (end2.tv_sec - end1.tv_sec) + (end2.tv_nsec - end1.tv_nsec) / 1.0e9);

    int weak_key_count = 0;

    mpz_t * weak_keys = (mpz_t * ) malloc(line_size * sizeof(mpz_t));

    for (int i = 0; i < line_size; i++) {
        mpz_init(weak_keys[i]);
    }
    // printf("init2 done\n");

    pthread_t threads[thread_num];
    ThreadData1 threadData[thread_num];

    int rem = line_size % thread_num;
    int load = line_size / thread_num;
    int fin_load = 0;
    int j = 0;
    int th = 0;

    for(th = 0 ; th < thread_num ; th++){
        if(rem > 0){
            fin_load = load+1;
            rem -= 1;
        }
        else{
            fin_load = load;
        }
        if(fin_load == 0){
            break;
        }

        threadData[th].current_row = array_of_arrays[0];
        threadData[th].prev_row = fileData;
        threadData[th].start_idx = j;
        threadData[th].end_idx = j + fin_load;
        threadData[th].weak_keys = weak_keys;
        threadData[th].weak_key_count = 0;
        pthread_create(&threads[th], NULL, weak_key, &threadData[th]);
        j += fin_load;

    }

    for(int p = 0 ; p < th ; p++){
        pthread_join(threads[p], NULL);
        weak_key_count += threadData[p].weak_key_count ;
    }

    clock_gettime(CLOCK_REALTIME, &end3);
    printf("[MAIN] %d Weak keys identified at: %f seconds (elapsed), duration: %f seconds\n", weak_key_count, (end3.tv_sec - start.tv_sec) + (end3.tv_nsec - start.tv_nsec) / 1.0e9, (end3.tv_sec - end2.tv_sec) + (end3.tv_nsec - end2.tv_nsec) / 1.0e9);

    // Cleanup -- to think about it after wards

    for (int i = 0; i < line_size; i++) {
        mpz_clear(weak_keys[i]);
    }
    free(weak_keys);

    for (int i = 0; i < line_size; i++) {
        mpz_clear(fileData[i]);
    }
    free(fileData);

    for (int i = 0, current_size = line_size; i < row - 1; i++, current_size = ceil(current_size / 2.0)) {
        for (int j = 0; j < current_size; j++) {
            mpz_clear(array_of_arrays[i][j]);
        }
        free(array_of_arrays[i]);
    }
    free(array_of_arrays);

    return 0;
}
