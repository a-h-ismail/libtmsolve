/*
Copyright (C) 2023 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifndef COMPLEX_H
#define COMPLEX_H
#include <complex.h>

/**
 * @file
 * @brief
 */

double complex tms_neglect_real_cmplx(double complex x);

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

double complex tms_clog(double complex z);
#endif