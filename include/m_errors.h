/*
Copyright (C) 2022-2024 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifndef M_ERRORS_H
#define M_ERRORS_H

/**
 * @file
 * @brief Contains all error messages.
 */

#define PARENTHESIS_MISSING "Expected ( for function call."
#define PARENTHESIS_EMPTY "Empty parenthesis pair."
#define PARENTHESIS_NOT_CLOSED "Open parenthesis has no closing parenthesis."
#define PARENTHESIS_NOT_OPEN "Extra closing parenthesis."
#define INVALID_MATRIX "Invalid matrix."
#define SYNTAX_ERROR "Syntax error."
#define INVALID_NAME "Invalid name, allowed characters: alphanumeric + underscore, starts with '_' or alphabetic."
#define ILLEGAL_NAME "Illegal name."
#define NO_FUNCTION_SHADOWING "Can't shadow built-in functions."
#define NO_INPUT "Empty input."
#define DIVISION_BY_ZERO "Division by zero isn't defined."
#define MODULO_ZERO "Modulo zero implies a division by zero."
#define MATH_ERROR "Math error."
#define RIGHT_OP_MISSING "Missing right operand."
#define UNDEFINED_VARIABLE "Undefined variable."
#define UNDEFINED_FUNCTION "Undefined function."
#define INTERNAL_ERROR "Internal error, please report this as a bug and include the expression that caused the issue."
#define MULTIPLE_ASSIGNMENT_ERROR "Multiple assignment operators are not supported."
#define OVERWRITE_CONST_VARIABLE "Overwriting read-only variables is not allowed."
#define ILLEGAL_COMPLEX_OP "Illegal complex operation."
#define TOO_MANY_ARGS "Too many arguments in function call."
#define TOO_FEW_ARGS "Too few arguments in function call."
#define EXTF_FAILURE "Extended function reported a failure."
#define INTEGRAl_UNDEFINED "Error: Ensure that the function is defined within the integration interval."
#define NOT_DERIVABLE "Error: Is this function defined around this point?"
#define COMPLEX_DISABLED "Complex value detected but complex is disabled."
#define COMPLEX_ONLY_FUNCTION "Function defined only in the complex domain."
#define MODULO_COMPLEX_NOT_SUPPORTED "Modulo operation for complex numbers is not supported."
#define NO_COMPLEX_LOG_BASE "Base-N logarithm doesn't support complex base."
#define NO_FSELF_REFERENCE "Can't reference a user function within itself."
#define NO_FCIRCULAR_REFERENCE "Circular function reference detected, function unchanged."
#define INTEGER_OVERFLOW "Warning: Integer overflow detected."
#define VAR_NAME_MATCHES_FUNCTION "Variable name can't shadow an existing function."
#define FUNCTION_NAME_MATCHES_VAR "Function name can't shadow an existing variable."
#define INT_TOO_LARGE "Value is too large for the current integer size."
#define MISSING_EXPRESSION "Assignment operator used, but no expression follows."
#define EXPRESSION_TOO_LONG "Expression is too large to be indexed using an integer."
#define NOT_A_VALID_IPV4 "An IPv4 should have four 8-bit fields."
#define NOT_A_DOT_DECIMAL "Each octet in a dot-decimal notation should be in range [0,255]."
#define NOT_AN_IPV4_SIZE "An IPv4 needs a variable size of 32 bits."
#define NOT_A_VALID_IPV4_PREFIX "An IPv4 prefix should be in range [0,32]."
#define SHIFT_TOO_LARGE "Shift count is too large for the current variable size."
#define SHIFT_AMOUNT_NEGATIVE "Shift amount can't be negative."
#define ROTATION_AMOUNT_NEGATIVE "Rotation amount can't be negative."
#define UNKNOWN_FUNC_ERROR "Function reported an error."
#define ERROR_DB_NOT_EMPTY "libtmsolve warning: expected an empty error database for this facility. Clearing it now..."
#define BIT_OUT_OF_RANGE "The bit to mask is out of range."
#endif