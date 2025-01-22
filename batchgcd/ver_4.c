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

typedef struct {
    mpz_t * div;
    mpz_t * op;
}
rem;

void * mod(void * arg) {
    mpz_t sq1, sq2;
    mpz_inits(sq1, sq2, NULL);
    rem * data = (rem * ) arg;
    mpz_mul(sq1, *(data -> op), *(data -> op));
    mpz_mod( * (data -> op), *(data -> div), sq1);
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
                    if (fin_load == 0) {
                        break;
                    }
                    rem -= 1;
                }
                else{
                    fin_load = load;
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

    mpz_t sq1, sq2;
    mpz_inits(sq1, sq2, NULL);
    printf("init1 done\n");

    for (int i = row - 2; i >= 0; i--) {
        printf("row size for %d is : %lld\n", i, row_size[i]);

        // THIS LOOP NEEDS MULTI PROCESSING 2
        for (int j = 0, k = 0; j < row_size[i]; j += 2, k += 1) {
            if (j + 2 <= row_size[i]) {
                mpz_mul(sq1, array_of_arrays[i][j], array_of_arrays[i][j]);
                mpz_mul(sq2, array_of_arrays[i][j + 1], array_of_arrays[i][j + 1]);

                mpz_mod(array_of_arrays[i][j], array_of_arrays[i + 1][k], sq1);
                mpz_mod(array_of_arrays[i][j + 1], array_of_arrays[i + 1][k], sq2);
            } else {
                mpz_mul(sq1, array_of_arrays[i][j], array_of_arrays[i][j]);
                mpz_mod(array_of_arrays[i][j], array_of_arrays[i + 1][k], sq1);
            }
        }
    }
    time( & end2);
    printf("[MAIN] Remainder Tree constructed by time: %f seconds\n", difftime(end2, start));

    int weak_key_count = 0;

    mpz_t one;
    mpz_init(one);
    mpz_set_str(one, "1", 16);

    mpz_t res;
    mpz_init(res);

    mpz_t * weak_keys = (mpz_t * ) malloc(line_size * sizeof(mpz_t));

    for (int i = 0; i < line_size; i++) {
        mpz_init(weak_keys[i]);
    }
    printf("init2 done\n");

    // THIS LOOP NEEDS MULTI PROCESSING 3
    for (int i = 0; i < line_size; i++) {
        mpz_div(res, array_of_arrays[0][i], fileData[i]);
        mpz_gcd(res, res, fileData[i]);
        if (mpz_cmp(res, one) != 0) {
            mpz_set(weak_keys[weak_key_count], fileData[i]);
            weak_key_count += 1;
        }
    }
    time( & end3);
    printf("[MAIN] %d Weak keys identified by time: %f seconds\n", weak_key_count, difftime(end3, start));

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

    // Clear the variables at the end
    mpz_clears(sq1, sq2, one, res, NULL);

    return 0;
}