#include <stdio.h>
#include <gmp.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#define thread_num 32

mpz_t * readFile(char * fileName, long * size) {
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
    mpz_t **aoa;
    long size;
    long rows;
    int tid;
} ThreadData;

void *multiply(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    mpz_t **matrix = data->aoa;
    // printf("thread load is: %ld\n", data->size);
    // gmp_printf("First element: %ZX\n", matrix[0][0]);

    // for(int i=0 ; i<data->size ;i++){
    //     if (mpz_cmp_ui(matrix[0][i], 0) == 0) {
    //         printf("we fked up at %d\n",i);
    //     }
    // }

    long prev = (data->size)/2;
    long row = 1;
    for (int j = ((data->size+1)/2); j >= 1; j = ((j+1)/2)) {
        if(row == ((data->rows)-1)){
            mpz_mul(matrix[row][0],matrix[row-1][0],matrix[row-1][1]);
            break;
        }
        // printf("j is: %d\n",j);
        if(prev != j){
            // printf("it is possible I guess\n");
            mpz_set(matrix[row][j-1], matrix[row-1][(prev*2)]);
        }

        for(int i=0 ; i<prev ;i++){
            mpz_mul(matrix[row][i],matrix[row-1][i*2],matrix[row-1][(i*2)+1]);
        }
       // for(int i=0 ; i<j ;i++){
       //     if (mpz_cmp_ui(matrix[row][i], 0) == 0) {
       //         printf("At thread %d we fked up at %d\n", data->tid, i);
       //     }
       // }

        prev = j/2;
        row++;
    }
    return NULL;
}

typedef struct {
    int num1;
    int num2;
    int result;
    mpz_t * old;
    mpz_t * new;
} ThreadData1;

void *multiply1(void *arg) {
    ThreadData1 *data = (ThreadData1 *)arg;
    mpz_mul(data->new[data->result], data->old[data->num1], data->old[data->num2]);
    return NULL;
}

// MAIN --------------------------------------------------------------------------------------------------------------------------------------------------------

int main() {
    struct timespec start, end, end1;
    long line_size;
    mpz_t * fileData = readFile("../input/input-4100k.txt", & line_size);
    if (!fileData) {
        return 1;
    }

    mpz_t *** thread_array = (mpz_t ***) malloc(thread_num * sizeof(mpz_t **));

    long rem = line_size % thread_num;
    long load = line_size / thread_num;
    long final_thread_load[thread_num];
    long row_size[thread_num];
    long total_load=0;

    printf("rem is: %ld\n", rem);
    printf("load is: %ld\n", load);

    for (int i = 0; i < thread_num; i++) {
        if(rem > 0){
            final_thread_load[i] = load + 1;
            rem -= 1;
        }
        else{
            final_thread_load[i] = load;
        }
        int row = 0;
        for (int j = final_thread_load[i]; j >= 1; j = ((j+1)/2)) {
            row++;
            printf("for row %d size is: %d\n", row, j);
            if (j == 1)
                break;
        }
        printf("for thread %d final thread load is: %ld with rows: %d \n", i, final_thread_load[i], row);
        row_size[i] = row;
        thread_array[i] = (mpz_t **) malloc(row * sizeof(mpz_t *));
        int itr = 0;
        for (int j = final_thread_load[i]; j >= 1; j = ((j+1)/2)) {
            thread_array[i][itr] = (mpz_t * ) malloc(j * sizeof(mpz_t));
            for (int k = 0; k < j; k++) {
                mpz_init(thread_array[i][itr][k]);
            }
            itr++;
            if (j == 1)
                break;
        }
        for (int j = 0 ; j < final_thread_load[i]; j++) {
            mpz_set(thread_array[i][0][j], fileData[j + total_load]);
        }
        total_load += final_thread_load[i];
    }    

    int aoa_row = 0;
    for (int j = thread_num; j >= 1; j = ((j+1)/2)) {
        aoa_row ++;
        if(j == 1){
            break;
        }
    }
    // printf("aoa_row size is: %d\n", aoa_row);

    mpz_t ** array_of_arrays = (mpz_t ** ) malloc((aoa_row) * sizeof(mpz_t * ));

    int itr = 0;
    for (int j = thread_num ; j >= 1; j = ((j+1)/2)) {
        array_of_arrays[itr] = (mpz_t * ) malloc(j * sizeof(mpz_t));
        for(int i = 0 ; i < j ; i++){
            mpz_init(array_of_arrays[itr][i]);
        }
        // printf("size of %d is: %d\n", itr, j);
        itr++;
        if(j == 1){
            break;
        }
    }

    printf("time counting started\n");

    clock_gettime(CLOCK_REALTIME, &start);

    pthread_t threads[thread_num];
    ThreadData threadData[thread_num];
    int th = 0;
    for (th = 0; th < thread_num; th++){
        if(final_thread_load[th] == 0){
            break;
        }
        threadData[th].aoa = thread_array[th];
        threadData[th].size = final_thread_load[th];
        threadData[th].rows = row_size[th];
        threadData[th].tid = th;
        pthread_create( & threads[th], NULL, multiply, & threadData[th]);
        // printf("thread %d created.\n",th);
    }
    //int th_cp = th;
    for (int p = 0; p < th; p++) {
        pthread_join(threads[p], NULL);
    }
    clock_gettime(CLOCK_REALTIME, &end);
    printf("[MAIN] half Product Tree constructed by time: %f seconds\n", (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1.0e9);
    
    // for (int i = 0; i < th_cp; i++) {
    //     gmp_printf("[MAIN] Thread %d result: %ZX, eval is: %d \n", i, thread_array[i][row_size[i]-1][0], row_size[i]-1);
    //     printf("\n");
    // }

    // gmp_printf("Thread %d result: %ZX\n", 0, thread_array[0][row_size[0]-1][0]);
    // printf("\n");

    for(int i=0 ; i<thread_num ; i++){
        mpz_set(array_of_arrays[0][i], thread_array[i][row_size[i]-1][0]);
        // printf("done %d\n",i);
    }

    // for(int i=0 ; i<thread_num ; i++){
    //     gmp_printf("[MAIN] column %d result: %ZX\n", i, array_of_arrays[0][i]);
    // }

    itr = 1;
    int prev = thread_num;
    for (int j = (thread_num+1)/2; j >= 1; j = ((j+1)/2)) {
        if(j == 1){
            mpz_mul(array_of_arrays[itr][0], array_of_arrays[itr-1][0], array_of_arrays[itr-1][1]);
            // printf("break pulled\n");
            break;
        }
        int q = prev/2;
        if(j != q){
            mpz_set(array_of_arrays[itr][q],array_of_arrays[itr-1][prev-1]);
            // printf("itr is %d this ran j is: %d prev is: %d this var %d is to this var %d\n", itr, j, prev, q, prev-1);
        }
        pthread_t threads[thread_num];
        ThreadData1 threadData1[thread_num];

        int th = 0;
        int var1 = 0, var2 = 0;
        for(; th< q ; th++){
            threadData1[th].num1 = var1;
            threadData1[th].num2 = var1+1;
            threadData1[th].result = var2;
            threadData1[th].old = array_of_arrays[itr-1];
            threadData1[th].new = array_of_arrays[itr];
            pthread_create( & threads[th], NULL, multiply1, & threadData1[th]);
            // printf("called thread %d with %d , %d as old to new %d\n",th ,var1, var1+1, var2);
            var1+=2;
            var2+=1;
        }
        for (int p = 0; p < th; p++) {
            pthread_join(threads[p], NULL);
        }
       // clock_gettime(CLOCK_REALTIME, &end1);
       // printf("[MAIN] itr %d Product Tree constructed by time: %f seconds\n", itr, (end1.tv_sec - start.tv_sec) + (end1.tv_nsec - start.tv_nsec) / 1.0e9);

        prev = j;
        // printf("itr is:%d j is:%d\n", itr, j);
        itr++;
    }
    clock_gettime(CLOCK_REALTIME, &end1);
    printf("[MAIN] Product Tree constructed by time: %f seconds\n", (end1.tv_sec - start.tv_sec) + (end1.tv_nsec - start.tv_nsec) / 1.0e9);
    // printf("itr is %d\n",itr);

    itr--;

    // for(; itr>=0 ; itr--){
    //     printf("itr is : %d\n",itr);
    // }

    // printf("itr is %d\n",itr);
    // gmp_printf("Thread %d result: %ZX\n", 0, array_of_arrays[itr][0]);
    // printf("\n");

    // mpz_t mul_result;
    // mpz_init(mul_result);
    // mpz_set_ui(mul_result, 1);

    // for (int i = 0; i < line_size; i++) {
    //     mpz_mul(mul_result, mul_result, fileData[i]);
    // }
    // // gmp_printf("mul result is result: %ZX\n", mul_result);
    // // printf("\n");

    // if (mpz_cmp(array_of_arrays[itr][0], mul_result) == 0) {
    //     printf("✅ Both results match!\n");
    // } else {
    //     printf("❌ Results do NOT match!\n");
    //     gmp_printf("Tree result: %ZX\n", array_of_arrays[itr][0]);
    //     gmp_printf("Flat  result: %ZX\n", mul_result);
    // }



    for (int i = 0; i < line_size; i++) {
        mpz_clear(fileData[i]);
    }
    free(fileData);

    return 0;
}
