/*
Copyright (C) 2023 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifndef _TMS_COMPLEX_H
#define _TMS_COMPLEX_H
#include <complex.h>

/**
 * @file Contains wrappers and missing complex functions from standard library
 * @brief
 */

/**
 * @brief Nullifies the real part if it is too small relative to the imaginary parts and vice versa.
 * @return The received value after approximation.
 */
double complex tms_neglect_real_cmplx(double complex z);

double complex tms_cexp(double complex z);

double complex tms_cpow(double complex x, double complex y);

double complex cabs_z(double complex z);

double complex carg_z(double complex z);

double complex tms_ccbrt(double complex z);

double complex tms_cceil(double complex z);

double complex tms_cfloor(double complex z);

double complex tms_cround(double complex z);

double complex tms_cfact(double complex z);

double complex tms_csign(double complex z);

double complex tms_cln(double complex z);

double complex tms_clog10(double complex z);

double complex tms_ccos(double complex z);

double complex tms_csin(double complex z);

double complex tms_ctan(double complex z);
#endif