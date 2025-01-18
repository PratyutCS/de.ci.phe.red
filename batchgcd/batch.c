#include <stdio.h>

#include <gmp.h>

#include <stdlib.h>

#include <math.h>

#include <time.h>

mpz_t * readFile(char * fileName, int * size) {
  //    printf("[READFILE] inputfileName is: %s\n", fileName);
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

  //    printf("[READFILE] line count is: %zd\n", line_count);
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

int main() {
  clock_t start, end;
  double cpu_time_used;
  start = clock();
  //	printf("[main] io works\n");
  int line_size;
  mpz_t * fileData = readFile("../input/input-100k.txt", & line_size);

  if (!fileData) {
    return 1;
  }

  int row = 0;
  for (double i = line_size; i >= 1.0; i = ceil(i / 2.0)) {
    row++;
    if (i == 1.0)
      break;
  }
  //    printf("[MAIN] rows required is: %d\n", row);

  mpz_t ** array_of_arrays = (mpz_t ** ) malloc(row * sizeof(mpz_t * ));

  for (int i = line_size, count = 0, prev = 0; i >= 1; i = ceil(i / 2.0), count += 1) {
    array_of_arrays[count] = (mpz_t * ) malloc(i * sizeof(mpz_t));
    //        printf("count is : %d i allocated is: %d prev is: %d\n", count, i, prev);

    for (int j = 0; j < i; j++) {
      mpz_init(array_of_arrays[count][j]);
    }

    if (i == line_size) {
      for (int j = 0; j < i; j++) {
        mpz_set(array_of_arrays[0][j], fileData[j]); // Deep copy fileData to array_of_arrays[0]
      }
    } else if (i == 1) {
      mpz_mul(array_of_arrays[count][0], array_of_arrays[count - 1][0], array_of_arrays[count - 1][1]);
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
  }
  printf("[MAIN] Product Tree constructed by time: %f seconds\n", ((double) clock() - start) / CLOCKS_PER_SEC);

  //    printf("[Main] Root Node of Product Tree is : %d",row-1);
  //    mpz_out_str(stdout, 16, array_of_arrays[row-1][0]);
  //    printf("\n");

  int weak_key_count = 0;

  mpz_t one;
  mpz_init(one);
  mpz_set_str(one, "1", 16);

  mpz_t sq;
  mpz_init(sq);

  mpz_t remainder;
  mpz_init(remainder);

  mpz_t rdp;
  mpz_init(rdp);

  mpz_t gcd;
  mpz_init(gcd);

  for (int i = 0; i < line_size; i++) {
    mpz_mul(sq, fileData[i], fileData[i]);
    mpz_mod(remainder, array_of_arrays[row - 1][0], sq);
    mpz_div(rdp, remainder, fileData[i]);
    mpz_gcd(gcd, rdp, fileData[i]);

    if (mpz_cmp(gcd, one) != 0) {
      weak_key_count += 1;
    }
    printf("weak keys: %d reached i: %d ETA: %f\n", weak_key_count, i, ((double) clock() - start) / CLOCKS_PER_SEC);
  }
  printf("[MAIN] Weak keys identified by time: %f seconds\n", ((double) clock() - start) / CLOCKS_PER_SEC);

  printf("[MAIN] weak key count is: %d\n", weak_key_count);

  // Cleanup

  for (int i = 0; i < line_size; i++) {
    mpz_clear(fileData[i]);
  }
  free(fileData);

  for (int i = 0, current_size = line_size; i < row; i++, current_size = ceil(current_size / 2.0)) {
    for (int j = 0; j < current_size; j++) {
      mpz_clear(array_of_arrays[i][j]);
    }
    free(array_of_arrays[i]);
  }
  free(array_of_arrays);

  return 0;
}
