#include <stdio.h>
#include <gmp.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>

int rows(int size){
  int count = 0;
  for(double i = size; i>=1.0; i=ceil((i/2.0))){
    count++;
    if(i == 1.0)
      break;
  }
  return count;
}

mpz_t* readFile(char* fileName, int* size){
  printf("[READFILE] inputfileName is:%s\n",fileName);
  FILE* inputFile = fopen(fileName, "r");

  char* line = NULL;
  size_t line_length = 0;
  size_t line_count = 0;

  while (getline(&line, &line_length, inputFile) != -1) {
    line_count++;
  }
  printf("[READFILE] line count is : %zd\n",line_count);
  mpz_t* array = (mpz_t*)malloc(line_count * sizeof(mpz_t));
  rewind(inputFile);

  for (size_t i = 0; i < line_count; i++) {
    mpz_init(array[i]);
    if (getline(&line, &line_length, inputFile) == -1) {
      perror("Error reading line");
      fclose(inputFile);
      free(line);
      for (size_t j = 0; j < i; j++) {
          mpz_clear(array[j]); // Free mpz_t resources
      }
      free(array);
      return NULL;
    }
    mpz_set_str(array[i], line, 16); // Parse the line as mpz_t
  }

  fclose(inputFile);
  free(line);
  *size = line_count;
  return array;
}

int main(){
  printf("[main] io works\n");
  int line_size;
  mpz_t* fileData;
  fileData = readFile("../input/input_sample.txt", &line_size);
//  mpz_out_str(stdout, 16, fileData[0]);
//  printf("\n");
//  mpz_out_str(stdout, 16, fileData[1]);
//  printf("\n");
//  mpz_t mul;
//  mpz_mul(mul, fileData[0], fileData[1]);
//  mpz_out_str(stdout, 16, mul);
//  printf("\n");
  int row = rows(line_size);
  printf("[MAIN] rows required is : %d\n",row);
  mpz_t** array_of_arrays = (mpz_t**)malloc(row * sizeof(mpz_t*));

  for (int i = line_size, count = 0, prev = 0; i >= 1; i = ceil(i / 2.0), count++, prev = i) {
     array_of_arrays[count] = (mpz_t*)malloc(i * sizeof(mpz_t));
     printf("i allocated is : %d\n",i);
     if(i == line_size){
       array_of_arrays[0] = fileData;
     }
    else if(i == 1)
       break;
    else{
      for (int j = 0, k = 0; j < i; j++, k += 2) {
        if(k==prev){
          printf("is this ever true ??");
          mpz_set(array_of_arrays[count][j], array_of_arrays[count-1][k]);
        }
        mpz_mul(array_of_arrays[count][j], array_of_arrays[count-1][k], array_of_arrays[count-1][k+1]);
      }
    }
  }
  free(array_of_arrays);
  free(fileData);
//  int sa = line_size;
//  for(int i=0;i<row;i++){
//    for(int j=0;i<sa;j++){
//      mpz_out_str(stdout, 16, array_of_arrays[i][j]);
//      printf(" --------- ");
//    }
//    printf("\n");
//    sa = ceil(sa/2.0);
//  }
  return 0;
}
