/*
Copyright (C) 2022-2023 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifndef TMSOLVE_H
#define TMSOLVE_H
/**
 * @file
 * @brief The main header of libtmsolve, includes all other headers for ease of use.
 */

#ifndef LOCAL_BUILD
#include <tmsolve/bitwise.h>
#include <tmsolve/error_handler.h>
#include <tmsolve/evaluator.h>
#include <tmsolve/function.h>
#include <tmsolve/int_parser.h>
#include <tmsolve/internals.h>
#include <tmsolve/matrix.h>
#include <tmsolve/parser.h>
#include <tmsolve/scientific.h>
#include <tmsolve/string_tools.h>
#include <tmsolve/tms_complex.h>
#include <tmsolve/tms_math_strs.h>
#include <tmsolve/version.h>
#else
#include "bitwise.h"
#include "error_handler.h"
#include "evaluator.h"
#include "function.h"
#include "int_parser.h"
#include "internals.h"
#include "matrix.h"
#include "parser.h"
#include "scientific.h"
#include "string_tools.h"
#include "tms_complex.h"
#include "tms_math_strs.h"
#include "version.h"
#endif

#endif