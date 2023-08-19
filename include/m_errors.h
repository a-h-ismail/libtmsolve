/*
Copyright (C) 2022-2023 Ahmad Ismail
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
#define ILLEGAL_VARIABLE_NAME "Illegal variable name."
#define NO_INPUT "Empty input."
#define DIVISION_BY_ZERO "Division by zero isn't defined."
#define MODULO_ZERO "Modulo zero implies a division by zero."
#define MATH_ERROR "Math error."
#define RIGHT_OP_MISSING "Missing right operand."
#define UNDEFINED_VARIABLE "Undefined variable."
#define UNDEFINED_FUNCTION "Undefined function."
#define INTERNAL_ERROR "Internal error, please report this as a bug and include the expression that caused the issue."
#define MULTIPLE_ASSIGNMENT_ERROR "Multiple assignment operators are not supported."
#define OVERWRITE_CONST_VARIABLE "Overwriting protected variables is not allowed."
#define ILLEGAL_COMPLEX_OP "Illegal complex operation."
#define TOO_MANY_ARGS "Too many arguments in function call."
#define TOO_FEW_ARGS "Too few arguments in function call."
#define EXTF_FAILURE "Extended function reported a failure."
#define INTEGRAl_UNDEFINED "Error while calculating integral, make sure the function is defined within the integration interval."
#define COMPLEX_DISABLED "Complex value detected but complex is disabled."
#define MODULO_COMPLEX_NOT_SUPPORTED "Modulo operation for complex numbers is not supported."
#endif