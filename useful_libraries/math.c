#include <lib/math.h>


uint32_t absi(int32_t input) 
{
    if (input > 0)
        return input;
    else
        return -input;
}

int32_t powi(int32_t base, uint32_t exp)
{
    if (base==0) return 0;
    else if ((base==1) || (exp==0)) return 1;

    int32_t val=base;
    /* init i with 1, because value already contains x^1 */
    for (uint32_t i=1;i<exp;i++) {
        val *= base;
    }
    
    return val;
}

int32_t divide_ceil(int32_t numerator, int32_t denominator)
{
    int32_t result = numerator / denominator;
    if (result*denominator < numerator)
        result++;

    return result;
}