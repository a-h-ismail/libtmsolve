/*
Copyright (C) 2021-2023 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/

/**
 * @file
 * @brief Contains all error messages.
 */
#ifndef M_ERRORS_H
#define M_ERRORS_H
#define PARENTHESIS_MISSING "Expected ( for function call."
#define PARENTHESIS_EMPTY "Empty parenthesis pair."
#define PARENTHESIS_NOT_CLOSED "Open parenthesis has no closing parenthesis."
#define PARENTHESIS_NOT_OPEN "Extra closing parenthesis."
#define INVALID_MATRIX "Invalid matrix."
#define SYNTAX_ERROR "Syntax error."
#define INVALID_VARIABLE_NAME "Invalid variable name, allowed characters: alphabetic + underscore."
#define NO_INPUT "Empty input."
#define DIVISION_BY_ZERO "Division by zero isn't defined."
#define MODULO_ZERO "Modulo zero implies a division by zero."
#define MATH_ERROR "Math error."
#define RIGHT_OP_MISSING "Missing right operand."
#define UNDEFINED_VARIABLE "Undefined variable."
#define UNDEFINED_FUNCTION "Undefined function."
#define INTERNAL_ERROR "Internal error, please report this as a bug and include the expression that caused the issue."
#define MULTIPLE_ASSIGNMENT_ERROR "Multiple assignment operators are not supported."
#define OVERWRITE_BUILTIN_VARIABLE "Overwriting built in variables is not allowed."
#define ILLEGAL_COMPLEX_OP "Illegal complex operation."
#endif