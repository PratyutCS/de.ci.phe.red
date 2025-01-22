#include <stdio.h>

#include <gmp.h>

#include <stdlib.h>

#include <math.h>

#include <time.h>

#include <pthread.h>

#define thread_num 15

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
    int start_idx;
    int end_idx;
    int thread;
}
ThreadData1;

void * finale(void *arg) {
    mpz_t one;
    mpz_init(one);
    mpz_set_str(one, "1", 16);

    mpz_t res;
    mpz_init(res);

    ThreadData1 *data = (ThreadData1 *) arg;

    int weak_key_count = 0;
    // int *weak_keys = malloc(sizeof(int));

    printf("THREAD is: %d - starting index is: %d - ending index is: %d - diff is: %d\n", data -> thread, data -> start_idx, data -> end_idx, data -> end_idx - data -> start_idx);

    for (int i = data -> start_idx ; i < data -> end_idx ; i++) {
        mpz_div(res, data -> current_row[i], data -> prev_row[i]);
        mpz_gcd(res, res, data -> prev_row[i]);
        if (mpz_cmp(res, one) != 0) {
            weak_key_count += 1;
        }
    }

    printf("THREAD is: %d - starting index is: %d - ending index is: %d - diff is: %d\n", data -> thread, data -> start_idx, data -> end_idx, data -> end_idx - data -> start_idx);
    mpz_clears(one, res, NULL);
    // *weak_keys = weak_key_count;

    return NULL;
}

// MAIN --------------------------------------------------------------------------------------------------------------------------------------------------------

int main() {
    time_t start, end1, end2, end3;
    //printf("[main] io works\n");
    int line_size;
    // CAN BE MULTI THREADED but is it beneficial ?
    mpz_t * fileData = readFile("../input/input-100k.txt", & line_size);

    if (!fileData) {
        return 1;
    }
    time( & start);

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
        array_of_arrays[count] = (mpz_t * ) malloc(i * sizeof(mpz_t));
        printf("count is : %d i allocated is: %d prev is: %d\n", count, i, prev);

        for (int j = 0; j < i; j++) {
            mpz_init(array_of_arrays[count][j]);
        }
        // ASSIGNMENT
        if (i == line_size) {
            for (int j = 0; j < i; j++) {
                mpz_set(array_of_arrays[count][j], fileData[j]);
            }

        } else if (i == 1) {
            mpz_mul(array_of_arrays[count][0], array_of_arrays[count - 1][0], array_of_arrays[count - 1][1]);
            row_size[count] = i;
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
        prev = i;
        row_size[count] = i;
    }
    time( & end1);
    printf("[MAIN] Product Tree constructed by time: %f seconds\n", difftime(end1, start));

    // mpz_out_str(stdout, 16, array_of_arrays[row-1][0]);

    // for (int i = 0; i < row; i++) {
    //   printf("row is: %d its value is: %lld\n", i, row_size[i]);
    // }

    for (int i = row - 2; i >= 0; i--) {
        printf("row size for %d is : %lld\n", i, row_size[i]);


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
    }
    time( & end2);
    printf("[MAIN] Remainder Tree constructed by time: %f seconds\n", difftime(end2, start));

    int weak_key_count = 0;

    // THIS LOOP NEEDS MULTI PROCESSING 3

    pthread_t threads[thread_num];
    ThreadData1 threadData[thread_num];

    int rem = line_size % thread_num;
    int load = line_size / thread_num;
    int fin_load = 0;
    int j = 0;
    int th = 0;

    for(int th = 0 ; th < thread_num ; th++){
        if(rem > 0){
            fin_load = load + 1;
            rem -= 1;
        }
        else{
            fin_load = load;
        }
        threadData[th].current_row = array_of_arrays[0];
        threadData[th].prev_row = fileData;
        threadData[th].start_idx = j;
        threadData[th].end_idx = j + fin_load;
        // THREAD CREATION
        // printf("fin_load is: %d - j is: %d - th is: %d - j+fin_load is: %d\n", fin_load, j, th, j+fin_load);
        threadData[th].thread = th;
        pthread_create( & threads[th], NULL, finale, & threadData[th]);
        j += fin_load;
    }

    for (int p = 0; p < th; p++) {
        void* return_value;
        pthread_join(threads[p], return_value);
        int weak_keys_from_thread = *(int *) return_value;
        weak_key_count += weak_keys_from_thread;
        free(return_value);
    }
    time( & end3);
    printf("[MAIN] %d Weak keys identified by time: %f seconds\n", weak_key_count, difftime(end3, start));

    // Cleanup -- to think about it after wards

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