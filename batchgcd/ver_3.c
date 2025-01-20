#include <stdio.h>

#include <gmp.h>

#include <stdlib.h>

#include <math.h>

#include <time.h>

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

// MAIN --------------------------------------------------------------------------------------------------------------------------------------------------------

int main() {
  clock_t start, end;
  double cpu_time_used;
  //printf("[main] io works\n");
  int line_size;
  mpz_t * fileData = readFile("../input/input-100k.txt", & line_size);

  if (!fileData) {
    return 1;
  }
  start = clock();

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
    // printf("count is : %d i allocated is: %d prev is: %d\n", count, i, prev);

    for (int j = 0; j < i; j++) {
      mpz_init(array_of_arrays[count][j]);
    }

    if (i == line_size) {
      for (int j = 0; j < i; j++) {
        mpz_set(array_of_arrays[count][j], fileData[j]);
      }

    } else if (i == 1) {
      mpz_mul(array_of_arrays[count][0], array_of_arrays[count - 1][0], array_of_arrays[count - 1][1]);
      row_size[count] = i;
      break;
    } else {
      for (int j = 0, k = 0; j < i; j += 1, k += 2) {
        if (k == prev - 1) {
          mpz_set(array_of_arrays[count][j], array_of_arrays[count - 1][k]);
        } else {
          mpz_mul(array_of_arrays[count][j], array_of_arrays[count - 1][k], array_of_arrays[count - 1][k + 1]);
        }
      }
    }
    prev = i;
    row_size[count] = i;
  }
  printf("[MAIN] Product Tree constructed by time: %f seconds\n", ((double) clock() - start) / CLOCKS_PER_SEC);

  // mpz_out_str(stdout, 16, array_of_arrays[row-2][0]);

  // for (int i = 0; i < row; i++) {
  //   printf("row is: %d its value is: %lld\n", i, row_size[i]);
  // }

  // Initialize sq1 and sq2 once at the start
  mpz_t sq1, sq2;
  mpz_inits(sq1, sq2, NULL);

  for (int i = row - 2; i >= 0; i--) {
    // printf("row size for %d is : %lld\n", i, row_size[i]);
    for (int j = 0, k = 0; j < row_size[i]; j += 2, k += 1) {
      if (j + 2 <= row_size[i]) {
        // printf("done for i: %d - j: %d - j+1: %d - k: %d\n", i, j, j+1, k);
        mpz_mul(sq1, array_of_arrays[i][j], array_of_arrays[i][j]);
        mpz_mul(sq2, array_of_arrays[i][j + 1], array_of_arrays[i][j + 1]);

        mpz_mod(array_of_arrays[i][j], array_of_arrays[i+1][k], sq1);
        mpz_mod(array_of_arrays[i][j + 1], array_of_arrays[i+1][k], sq2);
      } else {
        // printf("else ran \n");
        mpz_mul(sq1, array_of_arrays[i][j], array_of_arrays[i][j]);
        mpz_mod(array_of_arrays[i][j], array_of_arrays[i+1][k], sq1);
      }
    }
  }

  printf("[MAIN] Remainder Tree constructed by time: %f seconds\n", ((double) clock() - start) / CLOCKS_PER_SEC);

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
  // printf("init done\n");

  for (int i = 0; i < line_size; i++) {
    mpz_div(res, array_of_arrays[0][i], fileData[i]);
    mpz_gcd(res, res, fileData[i]);
    if (mpz_cmp(res, one) != 0) {
      mpz_set(weak_keys[weak_key_count], fileData[i]);
      weak_key_count += 1;
    }
  }
  printf("[MAIN] %d Weak keys identified by time: %f seconds\n", weak_key_count, ((double) clock() - start) / CLOCKS_PER_SEC);

  //(mpz_t *) realloc(weak_key_count * size_of(mpz_t)); // optional

  // Cleanup

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