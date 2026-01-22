/*
Copyright (C) 2023,2025-2026 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifndef _TMS_COMPLEX_H
#define _TMS_COMPLEX_H
#ifndef LOCAL_BUILD
#include <tmsolve/c_complex_to_cpp.h>
#else
#include "c_complex_to_cpp.h"
#endif

/**
 * @file Contains wrappers and complex functions not included in the standard library
 */

/**
 * @brief Nullifies the real part if it is too small relative to the imaginary parts and vice versa.
 * @return The received value after approximation.
 */

cdouble tms_round_to_axis(cdouble z);

cdouble tms_cexp(cdouble z);

cdouble tms_cpow(cdouble x, cdouble y);

cdouble cabs_z(cdouble z);

cdouble carg_z(cdouble z);

cdouble tms_ccbrt(cdouble z);

cdouble tms_cceil(cdouble z);

cdouble tms_cfloor(cdouble z);

double complex tms_round_to_zero(double complex z);

cdouble tms_cround(cdouble z);

cdouble tms_cfact(cdouble z);

cdouble tms_csign(cdouble z);

cdouble tms_cln(cdouble z);

cdouble tms_clog2(cdouble z);

cdouble tms_clog10(cdouble z);

cdouble tms_ccos(cdouble z);

cdouble tms_csin(cdouble z);

cdouble tms_ctan(cdouble z);

int tms_iscnan(cdouble z);
#endif