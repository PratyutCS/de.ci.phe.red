#include <stdio.h>
#include <stdlib.h>
#include <gmp.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

// Structure to pass arguments to the multiplication threads
typedef struct {
    mpz_t *result;
    const mpz_t *a;
    const mpz_t *b;
    int depth;
    int max_depth;
} mul_thread_args;

// Function prototypes
void *hybrid_mul_thread_func(void *args);
void hybrid_mul(mpz_t result, const mpz_t x, const mpz_t y, int depth, int max_depth);

// Thread function to call hybrid_mul recursively
void *hybrid_mul_thread_func(void *args) {
    mul_thread_args *thread_args = (mul_thread_args *)args;
    hybrid_mul(*thread_args->result, *thread_args->a, *thread_args->b, thread_args->depth, thread_args->max_depth);
    return NULL;
}

void hybrid_mul(mpz_t result, const mpz_t x, const mpz_t y, int depth, int max_depth) {
    // Base case: if we are at or beyond the max depth,
    // use the highly optimized GMP multiplication.
    if (depth >= max_depth) {
        mpz_mul(result, x, y);
        return;
    }

    // Determine split point (size of the numbers)
    size_t x_bits = mpz_sizeinbase(x, 2);
    size_t y_bits = mpz_sizeinbase(y, 2);
    
    // If numbers are too small to split, fallback to gmp_mul to avoid issues.
    // This threshold can be adjusted, but 2 bits is a safe minimum for splitting.
    if (x_bits < 2 || y_bits < 2) {
        mpz_mul(result, x, y);
        return;
    }

    size_t m = (x_bits > y_bits ? x_bits : y_bits) / 2;

    // Split x into a and b
    mpz_t a, b;
    mpz_init(a);
    mpz_init(b);
    mpz_tdiv_q_2exp(a, x, m); // a = x / 2^m
    mpz_tdiv_r_2exp(b, x, m); // b = x % 2^m

    // Split y into c and d
    mpz_t c, d;
    mpz_init(c);
    mpz_init(d);
    mpz_tdiv_q_2exp(c, y, m); // c = y / 2^m
    mpz_tdiv_r_2exp(d, y, m); // d = y % 2^m

    // The three recursive multiplications of Karatsuba
    mpz_t z0, z1, z2;
    mpz_init(z0);
    mpz_init(z1);
    mpz_init(z2);

    // Create thread arguments for the three sub-problems
    mul_thread_args args0, args1, args2;
    pthread_t tid0, tid1, tid2;

    // z0 = b * d
    args0.result = &z0;
    args0.a = &b;
    args0.b = &d;
    args0.depth = depth + 1;
    args0.max_depth = max_depth;
    
    // z2 = a * c
    args2.result = &z2;
    args2.a = &a;
    args2.b = &c;
    args2.depth = depth + 1;
    args2.max_depth = max_depth;

    // z1 = (a + b) * (c + d)
    mpz_t a_plus_b, c_plus_d;
    mpz_init(a_plus_b);
    mpz_init(c_plus_d);
    mpz_add(a_plus_b, a, b);
    mpz_add(c_plus_d, c, d);
    args1.result = &z1;
    args1.a = &a_plus_b;
    args1.b = &c_plus_d;
    args1.depth = depth + 1;
    args1.max_depth = max_depth;

    // Create and run threads to perform the sub-multiplications recursively
    pthread_create(&tid0, NULL, hybrid_mul_thread_func, &args0);
    pthread_create(&tid1, NULL, hybrid_mul_thread_func, &args1);
    pthread_create(&tid2, NULL, hybrid_mul_thread_func, &args2);

    // Wait for threads to finish
    pthread_join(tid0, NULL);
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);

    // Combine results: result = z2 * 2^(2m) + (z1 - z2 - z0) * 2^m + z0
    mpz_sub(z1, z1, z2);
    mpz_sub(z1, z1, z0);

    mpz_mul_2exp(z2, z2, 2 * m);
    mpz_mul_2exp(z1, z1, m);

    mpz_add(result, z2, z1);
    mpz_add(result, result, z0);

    // Clean up
    mpz_clear(a);
    mpz_clear(b);
    mpz_clear(c);
    mpz_clear(d);
    mpz_clear(z0);
    mpz_clear(z1);
    mpz_clear(z2);
    mpz_clear(a_plus_b);
    mpz_clear(c_plus_d);
}


