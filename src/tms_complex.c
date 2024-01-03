/*
Copyright (C) 2023 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "tms_complex.h"
#include "internals.h"
#include "m_errors.h"
#include "scientific.h"
#include <math.h>

double complex tms_neglect_real_cmplx(double complex z)
{
    double magnitude = creal(z) / cimag(z);
    if (fabs(magnitude) > 1e10)
        return creal(z);
    else if (fabs(magnitude) < 1e-10)
        return I * cimag(z);
    else
        return z;
}

double complex tms_cexp(double complex z)
{
    return tms_neglect_real_cmplx(cexp(z));
}

double complex tms_cpow(double complex x, double complex y)
{
    double x_real = creal(x), x_imag = cimag(x), y_real = creal(y), y_imag = cimag(y);
    if (y == 0)
        return 1;

    if (y_imag == 0)
    {
        if (x_imag == 0)
        {
            // n+0.5 powers for negative values returns a small real part error
            // Fixable by using sqrt()
            if (x_real < 0)
            {
                double y_real_int, y_real_decimal;
                if (y_real > 0)
                {
                    y_real_int = floor(y_real);
                    y_real_decimal = y_real - y_real_int;
                }
                else
                {
                    y_real_int = ceil(y_real);
                    y_real_decimal = y_real - y_real_int;
                }
                if (y_real_decimal == 0.5)
                    return pow(x_real, y_real_int) * I * sqrt(-x_real);
                else if (y_real_decimal == -0.5)
                    return 1 / (pow(x_real, y_real_int) * I * sqrt(-x_real));
            }
            else
                return pow(x_real, y_real);
        }
    }

    // Normal case
    return tms_neglect_real_cmplx(cpow(x, y));
}

// Some simple wrapper functions.
double complex cabs_z(double complex z)
{
    return cabs(z);
}

double complex carg_z(double complex z)
{
    return carg(z);
}

double complex tms_ccbrt(double complex z)
{
    return tms_cpow(z, 1.0 / 3);
}

double complex tms_cceil(double complex z)
{
    return ceil(creal(z)) + I * ceil(cimag(z));
}

double complex tms_cfloor(double complex z)
{
    return floor(creal(z)) + I * floor(cimag(z));
}

double complex tms_cround(double complex z)
{
    return round(creal(z)) + I * round(cimag(z));
}

double complex tms_cfact(double complex z)
{
    if (cimag(z) != 0)
        return NAN;
    else
        return tms_fact(creal(z));
}

double complex tms_csign(double complex z)
{
    if (cimag(z) != 0)
        return NAN;
    else
        return tms_sign(creal(z));
}

double complex tms_cln(double complex z)
{
    return clog(z);
}

double complex tms_clog(double complex z)
{
    return clog(z) / log(10);
}

double complex tms_ccos(double complex z)
{
    if (cimag(z) == 0)
        return tms_cos(creal(z));
    else
        return tms_neglect_real_cmplx(ccos(z));
}

double complex tms_csin(double complex z)
{
    if (cimag(z) == 0)
        return tm_sin(creal(z));
    else
        return tms_neglect_real_cmplx(csin(z));
}

double complex tms_ctan(double complex z)
{
    if (cimag(z) == 0)
        return tms_tan(creal(z));
    else
        return tms_neglect_real_cmplx(ctan(z));
}
