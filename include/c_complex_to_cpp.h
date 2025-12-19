#ifndef CCOMPLEX_TO_CPP
#define CCOMPLEX_TO_CPP

#ifdef __cplusplus
struct cpp_double
{
    double r, c;
};
#define cdouble cpp_double
#else
#include <complex.h>
#define cdouble double complex
#endif

#endif