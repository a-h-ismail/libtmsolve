/*
Copyright (C) 2021-2022 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifndef TMSOLVE_H
#define TMSOLVE_H
/**
 * @file
 * @brief The main header of libtmsolve, includes all other headers for ease of use.
*/

#ifndef LOCAL_BUILD
#include <tmsolve/scientific.h>
#include <tmsolve/string_tools.h>
#include <tmsolve/internals.h>
#include <tmsolve/function.h>
#include <tmsolve/matrix.h>
#include <tmsolve/m_errors.h>
#else
#include "scientific.h"
#include "string_tools.h"
#include "internals.h"
#include "function.h"
#include "matrix.h"
#include "m_errors.h"
#endif
#endif