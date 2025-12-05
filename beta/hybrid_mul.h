#ifndef HYBRID_MUL_H
#define HYBRID_MUL_H

#include <gmp.h>

// Function prototype for hybrid_mul
void hybrid_mul(mpz_t result, const mpz_t x, const mpz_t y, int depth, int max_depth);

#endif // HYBRID_MUL_H
