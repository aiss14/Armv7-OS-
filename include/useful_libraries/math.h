#include <stdio.h>

#ifndef MATH_H
#define MATH_H

uint32_t absi(int32_t input);
int32_t powi(int32_t base, uint32_t exp);

/* 
Division of integers where the result is rounded up.
Be cautions with negative values:
    divide_ceil( 5, 4) =>  2
    divide_ceil(-5, 4) => -1
    divide_ceil( 5,-4) => -1
*/
int32_t divide_ceil(int32_t numerator, int32_t denominator);

#endif // MATH_H