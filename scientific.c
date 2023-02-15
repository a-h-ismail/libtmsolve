/*
Copyright (C) 2021-2023 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "scientific.h"
#include "internals.h"
#include "function.h"
#include "string_tools.h"

int compare_subexps_depth(const void *a, const void *b)
{
    if ((*((m_subexpr *)a)).depth < (*((m_subexpr *)b)).depth)
        return 1;
    else if ((*((m_subexpr *)a)).depth > (*((m_subexpr *)b)).depth)
        return -1;
    else
        return 0;
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
double complex ccbrt_cpow(double complex z)
{
    return cpow(z, 1 / 3);
}

// Wrapper functions for cos, sin and tan to round for very small values

double rd_cos(double __x)
{
    __x = cos(__x);
    if (fabs(__x) < 1e-10)
        return 0;
    else
        return __x;
}

double rd_sin(double __x)
{
    __x = sin(__x);
    if (fabs(__x) < 1e-10)
        return 0;
    else
        return __x;
}

double rd_tan(double __x)
{
    __x = sin(__x);
    if (fabs(__x) < 1e-10)
        return 0;
    else
        return __x;
}

char *r_function_name[] =
    {"fact", "abs", "ceil", "floor", "sqrt", "cbrt", "acosh", "asinh", "atanh", "acos", "asin", "atan", "cosh", "sinh", "tanh", "cos", "sin", "tan", "ln", "log", NULL};
double (*r_function_ptr[])(double) =
    {factorial, fabs, ceil, floor, sqrt, cbrt, acosh, asinh, atanh, acos, asin, atan, cosh, sinh, tanh, rd_cos, rd_sin, rd_tan, log, log10};
// Extended functions, may take more than one parameter (stored in a comma separated string)
char *ext_function_name[] = {"int", "der", NULL};
double (*ext_math_function[])(char *) =
    {integrate, derivative};

// Complex functions
char *cmplx_function_name[] =
    {"abs", "arg", "sqrt", "cbrt", "acosh", "asinh", "atanh", "acos", "asin", "atan", "cosh", "sinh", "tanh", "cos", "sin", "tan", "log", NULL};
double complex (*cmplx_function_ptr[])(double complex) =
    {cabs_z, carg_z, csqrt, ccbrt_cpow, cacosh, casinh, catanh, cacos, casin, catan, ccosh, csinh, ctanh, ccos, csin, ctan, clog};

double factorial(double value)
{
    double result = 1;
    for (int i = 2; i <= value; ++i)
        result *= i;
    return result;
}

double complex calculate_expr(char *expr, bool enable_complex)
{
    double complex result;
    math_expr *math_struct;

    if (pre_parse_routine(expr) == false)
        return NAN;
    math_struct = parse_expr(expr, false, enable_complex);
    if (math_struct == NULL)
        return NAN;
    result = eval_math_expr(math_struct);
    delete_math_expr(math_struct);
    return result;
}

double complex calculate_expr_auto(char *expr)
{
    char *real_exclusive[] = {"%"};
    int i, j;
    // 0: can't be complex due to real exclusive operations like modulo.
    // 1: can be complex, mostly 2n roots of negative numbers.
    // 2: Is complex, imaginary number appears in the expression, or complex exclusive operations like arg().
    uint8_t likely_complex = 1;

    if (!pre_parse_routine(expr))
        return NAN;

    // Look for real exclusive operations
    for (i = 0; i < array_length(real_exclusive); ++i)
    {
        j = f_search(expr, real_exclusive[i], 0, false);
        if (j != -1)
        {
            likely_complex = 0;
            break;
        }
    }
    // We have some hope that this is a complex expression
    i = f_search(expr, "i", 0, true);
    if (i != -1)
    {
        // The complex number was found despite having a real expression, report the problem.
        if (likely_complex == 0)
        {
            error_handler(ILLEGAL_COMPLEX_OP, 1, 1, i);
            return NAN;
        }
        else
            likely_complex = 2;
    }

    math_expr *M;
    double complex result;
    switch (likely_complex)
    {
    case 0:
    {
        M = parse_expr(expr, false, false);
        result = eval_math_expr(M);
        delete_math_expr(M);
        return result;
    }
    case 1:
        M = parse_expr(expr, false, false);
        result = eval_math_expr(M);
        if (isnan(creal(result)))
        {
            // Check if the errors are fatal (like malformed syntax, division by zero...)
            int fatal_error = error_handler(NULL, 5, 1);
            if (fatal_error > 0)
                return NAN;

            // Clear errors caused by the first evaluation.
            error_handler(NULL, 3, 0);

            // The errors weren't fatal, possibly a complex answer.
            // Convert the math_struct to use complex functions.
            if (M != NULL)
            {
                convert_real_to_complex(M);
                // If the conversion failed due to real only functions, return a failure.
                if (M->enable_complex == false)
                {
                    delete_math_expr(M);
                    return NAN;
                }
            }
            else
                M = parse_expr(expr, false, true);

            result = eval_math_expr(M);
            delete_math_expr(M);
            return result;
        }
        else
        {
            delete_math_expr(M);
            return result;
        }
    case 2:
        M = parse_expr(expr, false, true);
        result = eval_math_expr(M);
        delete_math_expr(M);
        return result;
    }
    // Unlikely to fall out of the switch.
    error_handler(INTERNAL_ERROR, 1, 1, 0);
    return NAN;
}

char *lookup_function_name(void *function, bool is_complex)
{
    int i;
    if (is_complex)
    {
        for (i = 0; i < array_length(cmplx_function_ptr); ++i)
        {
            if (function == (void *)(cmplx_function_ptr[i]))
                break;
        }
        if (i < array_length(cmplx_function_ptr))
            return cmplx_function_name[i];
    }
    else
    {
        for (i = 0; i < array_length(r_function_ptr); ++i)
        {
            if (function == (void *)(r_function_ptr[i]))
                break;
        }
        if (i < array_length(r_function_ptr))
            return r_function_name[i];
    }

    return NULL;
}

void *lookup_function_pointer(char *function_name, bool is_complex)
{
    int i;
    if (is_complex)
    {
        for (i = 0; i < array_length(cmplx_function_name); ++i)
        {
            if (strcmp(cmplx_function_name[i], function_name) == 0)
                break;
        }
        if (i < array_length(cmplx_function_name))
            return cmplx_function_ptr[i];
    }
    else
    {
        for (i = 0; i < array_length(r_function_name); ++i)
        {
            if (strcmp(r_function_name[i], function_name) == 0)
                break;
        }
        if (i < array_length(r_function_name))
            return r_function_ptr[i];
    }

    return NULL;
}

void convert_real_to_complex(math_expr *M)
{
    // You need to swap real functions for their complex counterparts.
    m_subexpr *S = M->subexpr_ptr;
    if (S != NULL)
    {
        int s_index;
        char *function_name;
        for (s_index = 0; s_index < M->subexpr_count; ++s_index)
        {
            // Lookup the name of the real function and find the equivalent function pointer
            function_name = lookup_function_name(S[s_index].function_ptr, false);
            if (function_name == NULL)
                return;
            S->cmplx_function_ptr = lookup_function_pointer(function_name, true);
        }
    }
    M->enable_complex = true;
}

double complex eval_math_expr(math_expr *M)
{
    // No NULL pointer dereference allowed.
    if (M == NULL)
        return NAN;

    op_node *i_node;
    int s_index = 0, s_count = M->subexpr_count;
    // Subexpression pointer to access the subexpression array.
    m_subexpr *S = M->subexpr_ptr;
    while (s_index < s_count)
    {
        // Case with special function
        if (S[s_index].subexpr_nodes == NULL)
        {
            char *args;
            if (S[s_index].execute_extended)
            {
                int length = S[s_index].solve_end - S[s_index].solve_start + 1;
                // Copy args from the expression to a separate array.
                args = malloc((length + 1) * sizeof(char));
                memcpy(args, _glob_expr + S[s_index].solve_start, length * sizeof(char));
                args[length] = '\0';
                // Calling the extended function
                **(S[s_index].s_result) = (*(S[s_index].ext_function_ptr))(args);
                if (isnan((double)**(S[s_index].s_result)))
                {
                    error_handler(MATH_ERROR, 1, 0, S[s_index].solve_start, -1);
                    return NAN;
                }
                S[s_index].execute_extended = false;
                free(args);
            }
            ++s_index;

            continue;
        }
        i_node = S[s_index].subexpr_nodes + S[s_index].start_node;

        if (S[s_index].op_count == 0)
            *(i_node->node_result) = i_node->left_operand;
        else
            while (i_node != NULL)
            {
                if (i_node->node_result == NULL)
                {
                    error_handler(INTERNAL_ERROR, 1, 1, -1);
                    return NAN;
                }
                switch (i_node->operator)
                {
                case '+':
                    *(i_node->node_result) = i_node->left_operand + i_node->right_operand;
                    break;

                case '-':
                    *(i_node->node_result) = i_node->left_operand - i_node->right_operand;
                    break;

                case '*':
                    *(i_node->node_result) = i_node->left_operand * i_node->right_operand;
                    break;

                case '/':
                    if (i_node->right_operand == 0)
                    {
                        error_handler(DIVISION_BY_ZERO, 1, 1, i_node->operator_index);
                        return NAN;
                    }
                    *(i_node->node_result) = i_node->left_operand / i_node->right_operand;
                    break;

                case '%':
                    if (i_node->right_operand == 0)
                    {
                        error_handler(MODULO_ZERO, 1, 1, i_node->operator_index);
                        return NAN;
                    }
                    *(i_node->node_result) = fmod(i_node->left_operand, i_node->right_operand);
                    break;

                case '^':
                    // Workaround for the edge case where something like (6i)^2 is producing a small imaginary part
                    if (cimag(i_node->right_operand) == 0 && round(creal(i_node->right_operand)) - creal(i_node->right_operand) == 0)
                    {
                        *(i_node->node_result) = 1;
                        int i;
                        if (creal(i_node->right_operand) > 0)
                            for (i = 0; i < creal(i_node->right_operand); ++i)
                                *(i_node->node_result) *= i_node->left_operand;

                        else if (creal(i_node->right_operand) < 0)
                            for (i = 0; i > creal(i_node->right_operand); --i)
                                *(i_node->node_result) /= i_node->left_operand;
                    }
                    else
                        *(i_node->node_result) = cpow(i_node->left_operand, i_node->right_operand);
                    break;
                }
                i_node = i_node->next;
            }
        // Executing function on the subexpression result
        // Complex function
        if (M->enable_complex && S[s_index].cmplx_function_ptr != NULL)
            **(S[s_index].s_result) = (*(S[s_index].cmplx_function_ptr))(**(S[s_index].s_result));
        // Real function
        else if (!M->enable_complex && S[s_index].function_ptr != NULL)
            **(S[s_index].s_result) = (*(S[s_index].function_ptr))(**(S[s_index].s_result));

        if (isnan((double)**(S[s_index].s_result)))
        {
            error_handler(MATH_ERROR, 1, 0, S[s_index].solve_start, -1);
            return NAN;
        }
        ++s_index;
    }
    // dump_expr_data(M);

    if (M->variable_index != -1)
        variable_values[M->variable_index] = M->answer;
    return M->answer;
}

bool set_variable_metadata(char *expr, op_node *x_node, char operand)
{
    bool is_negative = false, is_variable = false;
    if (*expr == 'x')
        is_variable = true;
    else if (expr[1] == 'x')
    {
        if (*expr == '+')
            is_variable = true;
        else if (*expr == '-')
            is_variable = is_negative = true;
        else
            return false;
    }
    // The value is not the variable x
    else
        return false;
    if (is_variable)
    {
        // x as left op
        if (operand == 'l')
        {
            if (is_negative)
                x_node->var_metadata = x_node->var_metadata | 0b101;
            else
                x_node->var_metadata = x_node->var_metadata | 0b1;
        }
        // x as right op
        else if (operand == 'r')
        {
            if (is_negative)
                x_node->var_metadata = x_node->var_metadata | 0b1010;
            else
                x_node->var_metadata = x_node->var_metadata | 0b10;
        }
        else
        {
            puts("Invaild operand argument.");
            exit(2);
        }
    }
    return true;
}

math_expr *parse_expr(char *expr, bool enable_variables, bool enable_complex)
{
    int i, j, length;
    // Number of subexpressions
    int s_count = 1;
    // Used for indexing of subexpressions
    int s_index;
    // Use for dynamic growing blocks.
    int buffer_size = 8;
    // Used to store the index of the variable to assign the answer to.
    int variable_index = -1;
    // Local expression may be offset compared to the expression due to the assignment operator (if it exists).
    char *local_expr;

    // Search for assignment operator, to enable user defined variables
    i = f_search(expr, "=", 0, false);
    if (i != -1)
    {
        if (i == 0)
        {
            error_handler(SYNTAX_ERROR, 1, 1, 0);
            return NULL;
        }
        else
        {
            // Check if another assignment operator is used
            j = f_search(expr, "=", i + 1, false);
            _glob_expr = expr;
            if (j != -1)
            {
                error_handler(MULTIPLE_ASSIGNMENT_ERROR, 1, 1, j);
                return NULL;
            }
            // Store the variable name in this array
            char tmp[i + 1];
            strncpy(tmp, expr, i);
            tmp[i] = '\0';
            for (j = 0; j < variable_count; ++j)
            {
                // An existing variable is found
                if (strcmp(variable_names[j], tmp) == 0)
                {
                    variable_index = j;
                    break;
                }
            }
            if (j < hardcoded_variable_count)
            {
                error_handler(OVERWRITE_BUILTIN_VARIABLE, 1, 1, i);
                return NULL;
            }
            // Create a new variable
            if (j == variable_count)
            {
                // Dynamically expand size
                if (variable_count == variable_max)
                {
                    variable_max *= 2;
                    variable_names = realloc(variable_names, variable_max * sizeof(char *));
                    variable_values = realloc(variable_values, variable_max * sizeof(double complex));
                }
                variable_names[variable_count] = tmp;
                variable_index = variable_count++;
            }
            local_expr = expr + i + 1;
        }
    }
    else
        local_expr = expr;
    _glob_expr = local_expr;
    length = strlen(local_expr);
    // Pointer to subexpressions heap array
    m_subexpr *S;
    op_node *node_block, *tmp_node;
    // Pointer to the math_expr generated
    math_expr *M = malloc(sizeof(math_expr));

    M->var_count = 0;
    M->var_data = NULL;
    M->enable_complex = enable_complex;

    /*
    Parsing steps:
    - Locate and determine the depth of each subexpression (the whole expression's "subexpression" has a depth of 0)
    - Fill the expression data (start/end...)
    - Sort subexpressions by depth (high to low)
    - Parse subexpressions from high to low depth
    */

    S = malloc(buffer_size * sizeof(m_subexpr));

    int depth = 0;
    s_index = 0;
    // Determining the depth and start/end of each subexpression parenthesis
    for (i = 0; i < length; ++i)
    {
        // Resize the subexpr array on the fly
        if (s_index == buffer_size)
        {
            buffer_size *= 2;
            S = realloc(S, buffer_size * sizeof(m_subexpr));
        }
        if (local_expr[i] == '(')
        {
            S[s_index].ext_function_ptr = NULL;
            // Treat extended functions as a subexpression
            for (j = 0; j < sizeof(ext_math_function) / sizeof(*ext_math_function); ++j)
            {
                int temp;
                temp = r_search(local_expr, ext_function_name[j], i - 1, true);
                if (temp != -1)
                {
                    S[s_index].subexpr_start = temp;
                    S[s_index].solve_start = i + 1;
                    i = find_closing_parenthesis(local_expr, i);
                    S[s_index].solve_end = i - 1;
                    S[s_index].depth = depth + 1;
                    S[s_index].ext_function_ptr = ext_math_function[j];
                    break;
                }
            }
            if (S[s_index].ext_function_ptr != NULL)
            {
                ++s_index;
                continue;
            }
            // Normal case
            ++depth;
            S[s_index].solve_start = i + 1;
            S[s_index].depth = depth;
            // The expression start is the parenthesis, may change if a function is found
            S[s_index].subexpr_start = i;
            S[s_index].solve_end = find_closing_parenthesis(local_expr, i) - 1;
            ++s_index;
        }
        else if (local_expr[i] == ')')
            --depth;
    }
    // + 1 for the subexpression with depth 0
    s_count = s_index + 1;
    // Shrink the block to the required size
    S = realloc(S, s_count * sizeof(m_subexpr));
    // Copy the pointer to the structure
    M->subexpr_ptr = S;

    M->subexpr_count = s_count;
    for (i = 0; i < s_count; ++i)
    {
        S[i].function_ptr = NULL;
        S[i].cmplx_function_ptr = NULL;
        S[i].subexpr_nodes = NULL;
        S[i].execute_extended = true;
    }

    // The whole expression's "subexpression"
    S[s_index].depth = 0;
    S[s_index].solve_start = S[s_index].subexpr_start = 0;
    S[s_index].solve_end = length - 1;
    S[s_index].ext_function_ptr = NULL;

    // Sort by depth (high to low)
    qsort(S, s_count, sizeof(m_subexpr), compare_subexps_depth);

    int *operator_index, solve_start, solve_end, op_count, status;
    buffer_size = 16;

    /*
    How to parse a subexpression:
    - Handle the special case of extended functions (example: int, der)
    - Create an array that determines the location of operators in the string
    - Check if the subexpression has a function to execute on the result, set pointer
    - Allocate the array of nodes
    - Use the operator index array to fill the nodes data on operators type and location
    - Fill nodes operator priority
    - Fill nodes with values or set as variable (using x)
    - Set nodes order of calculation (using *next)
    - Set the result pointer of each op_node relying on its position and neighbor priorities
    - Set the subexpr result double pointer to the result pointer of the last calculated op_node
    */
    for (s_index = 0; s_index < s_count; ++s_index)
    {
        if (S[s_index].ext_function_ptr != NULL)
        {
            S[s_index].subexpr_nodes = NULL;
            S[s_index].s_result = malloc(sizeof(double complex *));
            continue;
        }

        // For simplicity
        solve_start = S[s_index].solve_start;
        solve_end = S[s_index].solve_end;

        operator_index = (int *)malloc(buffer_size * sizeof(int));
        // Count number of operators and store it's indexes
        for (i = solve_start, op_count = 0; i <= solve_end; ++i)
        {
            // Skipping over an already processed expression
            if (local_expr[i] == '(')
            {
                int previous_subexp;
                previous_subexp = find_subexpr_by_start(S, i + 1, s_index, 2);
                if (previous_subexp != -1)
                    i = S[previous_subexp].solve_end + 1;
            }
            else if (is_digit(local_expr[i]))
                continue;

            else if (is_op(local_expr[i]) && i != solve_start)
            {
                // Skipping a + or - used in scientific notation (like 1e+5)
                if (i > 0 && (local_expr[i - 1] == 'e' || local_expr[i - 1] == 'E') && (local_expr[i] == '+' || local_expr[i] == '-'))
                {
                    ++i;
                    continue;
                }
                // Varying the array size on demand
                if (op_count == buffer_size)
                {
                    buffer_size *= 2;
                    operator_index = realloc(operator_index, buffer_size * sizeof(int));
                }
                operator_index[op_count] = i;
                // Skipping a + or - coming just after an operator
                if (local_expr[i + 1] == '-' || local_expr[i + 1] == '+')
                    ++i;
                ++op_count;
            }
        }

        // Searching for any function preceding the expression to set the function pointer
        if (solve_start > 1 && (is_alphabetic(expr[solve_start - 2]) || expr[solve_start - 2] == '_'))
        {
            if (!enable_complex)
            {
                // Determining the number of functions to check at runtime
                int total = sizeof(r_function_ptr) / sizeof(*r_function_ptr);
                for (i = 0; i < total; ++i)
                {
                    j = r_search(local_expr, r_function_name[i], solve_start - 2, true);
                    if (j != -1)
                    {
                        S[s_index].function_ptr = r_function_ptr[i];
                        // Setting the start of the subexpression to the start of the function name
                        S[s_index].subexpr_start = j;
                        break;
                    }
                }
                // The function isn't defined for real operations
                if (S[s_index].function_ptr == NULL)
                {
                    error_handler(UNDEFINED_FUNCTION, 1, 0, solve_start - 2);
                    delete_math_expr(M);
                    free(operator_index);
                    return NULL;
                }
            }
            else
            {
                // Determining the number of complex functions to check
                int total = sizeof(cmplx_function_ptr) / sizeof(*cmplx_function_ptr);
                for (i = 0; i < total; ++i)
                {
                    j = r_search(local_expr, cmplx_function_name[i], solve_start - 2, true);
                    if (j != -1)
                    {
                        S[s_index].cmplx_function_ptr = cmplx_function_ptr[i];
                        S[s_index].subexpr_start = j;
                        break;
                    }
                }
                // The function is not defined in the complex domain
                if (S[s_index].cmplx_function_ptr == NULL)
                {
                    error_handler(UNDEFINED_FUNCTION, 1, 0, solve_start - 2);
                    free(operator_index);
                    delete_math_expr(M);
                    return NULL;
                }
            }
        }

        S[s_index].op_count = op_count;
        // Allocating the array of nodes
        // Case of no operators, create a dummy op_node and store the value in op1
        if (op_count == 0)
            S[s_index].subexpr_nodes = malloc(sizeof(op_node));
        else
            S[s_index].subexpr_nodes = malloc(op_count * sizeof(op_node));

        node_block = S[s_index].subexpr_nodes;

        // Checking if the expression is terminated with an operator
        if (op_count != 0 && operator_index[op_count - 1] == solve_end)
        {
            error_handler(RIGHT_OP_MISSING, 1, 1, operator_index[op_count - 1]);
            delete_math_expr(M);
            free(operator_index);
            return NULL;
        }
        // Filling operations and index data into each op_node
        for (i = 0; i < op_count; ++i)
        {
            node_block[i].operator_index = operator_index[i];
            node_block[i].operator= local_expr[operator_index[i]];
        }
        free(operator_index);

        // Case of expression with one term, use one op_node with operand1 to hold the number
        if (op_count == 0)
        {
            i = find_subexpr_by_start(S, S[s_index].solve_start, s_index, 1);
            S[s_index].subexpr_nodes[0].var_metadata = 0;
            // Case of nested no operators expressions, set the result of the deeper expression as the left op of the dummy
            if (i != -1)
                *(S[i].s_result) = &(node_block->left_operand);
            else
            {
                node_block->left_operand = read_value(local_expr, solve_start, enable_complex);
                if (isnan((double)node_block->left_operand))
                {
                    status = set_variable_metadata(local_expr + solve_start, node_block, 'l');
                    if (!status)
                    {
                        error_handler(UNDEFINED_VARIABLE, 1, 1, solve_start);
                        delete_math_expr(M);
                        return NULL;
                    }
                }
            }

            S[s_index].start_node = 0;
            S[s_index].s_result = &(node_block->node_result);
            node_block[0].next = NULL;
            // If the one term expression is the last one, use the math_struct answer
            if (s_index == s_count - 1)
            {
                node_block[0].node_result = &M->answer;
                break;
            }
            continue;
        }
        else
        {
            // Filling each op_node's priority data
            priority_fill(node_block, op_count);
            // Filling nodes with operands, filling operands index and setting var_metadata to 0
            for (i = 0; i < op_count; ++i)
            {
                node_block[i].node_index = i;
                node_block[i].var_metadata = 0;
            }
        }

        // Reading the first number
        node_block[0].left_operand = read_value(local_expr, solve_start, enable_complex);
        // If reading a value fails, it is probably a variable like x
        if (isnan((double)node_block[0].left_operand))
        {
            // Checking for the variable 'x'
            if (enable_variables == true)
                status = set_variable_metadata(local_expr + solve_start, node_block, 'l');
            else
                status = false;
            // Variable x not found, the left operand is probably another subexpression
            if (!status)
            {
                status = find_subexpr_by_start(S, solve_start, s_index, 1);
                if (status == -1)
                {
                    error_handler(UNDEFINED_VARIABLE, 1, 1, solve_start);
                    delete_math_expr(M);
                    return NULL;
                }
                else
                    *(S[status].s_result) = &(node_block[0].left_operand);
            }
        }

        // Intermediate nodes, read number to the appropriate op_node operand
        for (i = 0; i < op_count - 1; ++i)
        {
            if (node_block[i].priority >= node_block[i + 1].priority)
            {
                node_block[i].right_operand = read_value(local_expr, node_block[i].operator_index + 1, enable_complex);
                // Case of reading the number to the right operand of a op_node
                if (isnan((double)node_block[i].right_operand))
                {
                    // Checking for the variable 'x'
                    if (enable_variables == true)
                        status = set_variable_metadata(local_expr + node_block[i].operator_index + 1, node_block + i, 'r');
                    else
                        status = false;
                    if (!status)
                    {
                        // Case of a subexpression result as right_operand, set its result pointer to right_operand
                        status = find_subexpr_by_start(S, node_block[i].operator_index + 1, s_index, 1);
                        if (status == -1)
                        {
                            error_handler(UNDEFINED_VARIABLE, 1, 1, node_block[i].operator_index + 1);
                            delete_math_expr(M);
                            return NULL;
                        }
                        else
                            *(S[status].s_result) = &(node_block[i].right_operand);
                    }
                }
            }
            // Case of the op_node[i] having less priority than the next op_node, use next op_node's left_operand
            else
            {
                // Read number
                node_block[i + 1].left_operand = read_value(local_expr, node_block[i].operator_index + 1, enable_complex);
                if (isnan((double)node_block[i + 1].left_operand))
                {
                    // Checking for the variable 'x'
                    if (enable_variables == true)
                        status = set_variable_metadata(local_expr + node_block[i].operator_index + 1, node_block + i + 1, 'l');
                    else
                        status = false;
                    // Again a subexpression
                    if (!status)
                    {
                        status = find_subexpr_by_start(S, node_block[i].operator_index + 1, s_index, 1);
                        if (status == -1)
                        {
                            error_handler(UNDEFINED_VARIABLE, 1, 1, node_block[i].operator_index + 1);
                            delete_math_expr(M);
                            return NULL;
                        }
                        else
                            *(S[status].s_result) = &(node_block[i + 1].left_operand);
                    }
                }
            }
        }
        // Placing the last number in the last op_node
        // Read a value
        node_block[op_count - 1].right_operand = read_value(local_expr, node_block[op_count - 1].operator_index + 1, enable_complex);
        if (isnan((double)node_block[op_count - 1].right_operand))
        {
            // Checking for the variable 'x'
            if (enable_variables == true)
                status = set_variable_metadata(local_expr + node_block[op_count - 1].operator_index + 1, node_block + op_count - 1, 'r');
            else
                status = false;
            if (!status)
            {
                // If an expression is the last term, find it and set the pointers
                status = find_subexpr_by_start(S, node_block[op_count - 1].operator_index + 1, s_index, 1);
                if (status == -1)
                {
                    error_handler(UNDEFINED_VARIABLE, 1, 1, node_block[i].operator_index + 1);
                    return NULL;
                }
                *(S[status].s_result) = &(node_block[op_count - 1].right_operand);
            }
        }
        // Set the starting op_node by searching the first op_node with the highest priority
        for (i = 3; i > 0; --i)
        {
            for (j = 0; j < op_count; ++j)
            {
                if (node_block[j].priority == i)
                {
                    S[s_index].start_node = j;
                    // Exiting the main loop by setting i outside continue condition
                    i = -1;
                    break;
                }
            }
        }

        // Setting nodes order of execution
        i = S[s_index].start_node;
        int target_priority = node_block[i].priority;
        j = i + 1;
        while (target_priority > 0)
        {
            // Running through the nodes to find a op_node with the target priority
            while (j < op_count)
            {
                if (node_block[j].priority == target_priority)
                {
                    node_block[i].next = node_block + j;
                    // The op_node found is used as te start op_node for the next run
                    i = j;
                }
                ++j;
            }
            --target_priority;
            j = 0;
        }
        // Setting the next pointer of the last op_node to NULL
        node_block[i].next = NULL;

        // Set result pointers for each op_node based on position and priority
        tmp_node = &(node_block[S[s_index].start_node]);
        int left_node, right_node, prev_index = -2, prev_left = -2, prev_right = -2;
        while (tmp_node->next != NULL)
        {
            i = tmp_node->node_index;
            // Finding the previous and next nodes to compare
            left_node = i - 1, right_node = i + 1;

            while (left_node != -1)
            {
                if (left_node == prev_index)
                {
                    left_node = prev_left;
                    break;
                }
                else if (tmp_node->priority <= node_block[left_node].priority)
                    --left_node;
                // Optimize the edge case of successive <= priority nodes like a+a+a+a+a+a+a+a by using previous results
                else
                    break;
            }

            while (right_node < op_count)
            {
                if (right_node == prev_index)
                {
                    right_node = prev_right;
                    break;
                }
                else if (tmp_node->priority < node_block[right_node].priority)
                    ++right_node;
                // Similar idea to the above
                else
                    break;
            }
            // Case of the first op_node or a op_node with no left candidate
            if (left_node == -1)
                tmp_node->node_result = &(node_block[right_node].left_operand);
            else if (right_node == op_count)
                tmp_node->node_result = &(node_block[left_node].right_operand);
            // Case of a op_node between 2 nodes
            else if (left_node > -1 && right_node < op_count)
            {
                if (node_block[left_node].priority >= node_block[right_node].priority)
                    tmp_node->node_result = &(node_block[left_node].right_operand);
                else
                    tmp_node->node_result = &(node_block[right_node].left_operand);
            }
            // Case of the last op_node
            else if (i == op_count - 1)
                tmp_node->node_result = &(node_block[left_node].right_operand);
            tmp_node = tmp_node->next;

            prev_index = i;
            prev_left = left_node;
            prev_right = right_node;
        }
        // Case of the last op_node in the traversal order, set result to be result of the subexpression
        S[s_index].s_result = &(tmp_node->node_result);
        // The last op_node in the last subexpression should point to math_struct answer
        if (s_index == s_count - 1)
            tmp_node->node_result = &M->answer;
    }
    // Set the variables metadata
    if (enable_variables)
        _set_var_data(M);

    if (local_expr != expr)
        M->variable_index = variable_index;
    else
        M->variable_index = -1;
    return M;
}

void delete_math_expr(math_expr *M)
{
    if (M == NULL)
        return;

    int i = 0;
    m_subexpr *subexps = M->subexpr_ptr;
    for (i = 0; i < M->subexpr_count; ++i)
    {
        if (subexps[i].ext_function_ptr != NULL)
            free(subexps[i].s_result);
        free(subexps[i].subexpr_nodes);
    }
    free(subexps);
    free(M->var_data);
    free(M);
}

int_factor *find_factors(int32_t value)
{
    int32_t dividend = 2;
    int_factor *factor_list;
    int i = 0;

    factor_list = calloc(64, sizeof(int_factor));
    if (value == 0)
        return factor_list;
    factor_list[0].factor = factor_list[0].power = 1;

    // Turning negative numbers into positive for factorization to work
    if (value < 0)
        value = -value;
    // Simple optimized factorization algorithm
    while (value != 1)
    {
        if (value % dividend == 0)
        {
            ++i;
            value = value / dividend;
            factor_list[i].factor = dividend;
            factor_list[i].power = 1;
            while (value % dividend == 0 && value >= dividend)
            {
                value = value / dividend;
                ++factor_list[i].power;
            }
        }
        else
        {
            // Optimize by skipping values greater than value/2 (in other terms, the value itself is a prime)
            if (dividend > value / 2 && value != 1)
            {
                ++i;
                factor_list[i].factor = value;
                factor_list[i].power = 1;
                break;
            }
        }
        // Optimizing factorization by skipping even values (> 2)
        if (dividend > 2)
            dividend = dividend + 2;
        else
            ++dividend;
    }
    // Case of a prime number
    if (i == 0)
    {
        factor_list[1].factor = value;
        factor_list[1].power = 1;
    }
    return factor_list;
}

void reduce_fraction(fraction *fraction_str)
{
    int i = 1, j = 1, min;
    int_factor *num_factor, *denom_factor;
    num_factor = find_factors(fraction_str->b);
    denom_factor = find_factors(fraction_str->c);
    // If a zero was returned by the factorization function, nothing to do.
    if (num_factor->factor == 0 || denom_factor->factor == 0)
    {
        free(num_factor);
        free(denom_factor);
        return;
    }
    while (num_factor[i].factor != 0 && denom_factor[j].factor != 0)
    {
        if (num_factor[i].factor == denom_factor[j].factor)
        {
            // Subtract the minimun from the power of the num and denom
            min = find_min(num_factor[i].power, denom_factor[j].power);
            num_factor[i].power -= min;
            denom_factor[j].power -= min;
            ++i;
            ++j;
        }
        else
        {
            // If the factor at i is smaller increment i , otherwise increment j
            if (num_factor[i].factor < denom_factor[j].factor)
                ++i;
            else
                ++j;
        }
    }
    fraction_str->b = fraction_str->c = 1;
    // Calculate numerator and denominator after reduction
    for (i = 1; num_factor[i].factor != 0; ++i)
        fraction_str->b *= pow(num_factor[i].factor, num_factor[i].power);

    for (i = 1; denom_factor[i].factor != 0; ++i)
        fraction_str->c *= pow(denom_factor[i].factor, denom_factor[i].power);

    free(num_factor);
    free(denom_factor);
}
// Converts a floating point value to decimal representation a*b/c
fraction decimal_to_fraction(double value, bool inverse_process)
{
    int decimal_point = 1, p_start, p_end, decimal_length;
    bool success = false;
    char pattern[11], printed_value[23];
    fraction result;

    // Using the denominator as a mean to report failure, if c==0 then the function failed
    result.c = 0;

    if (fabs(value) > INT32_MAX || fabs(value) < pow(10, -log10(INT32_MAX) + 1))
        return result;

    // Store the integer part in 'a'
    result.a = floor(value);
    value -= floor(value);

    // Edge case due to floating point lack of precision
    if (1 - value < 1e-9)
        return result;

    // The case of an integer
    if (value == 0)
        return result;

    sprintf(printed_value, "%.14lf", value);
    // Removing trailing zeros
    for (int i = strlen(printed_value) - 1; i > decimal_point; --i)
    {
        // Stop at the first non zero value and null terminate
        if (*(printed_value + i) != '0')
        {
            *(printed_value + i + 1) = '\0';
            break;
        }
    }

    decimal_length = strlen(printed_value) - decimal_point - 1;
    if (decimal_length >= 10)
    {
        // Look for a pattern to detect fractions from periodic decimals
        for (p_start = decimal_point + 1; p_start - decimal_point < decimal_length / 2; ++p_start)
        {
            // First number in the pattern (to the right of the decimal_point)
            pattern[0] = printed_value[p_start];
            pattern[1] = '\0';
            p_end = p_start + 1;
            while (true)
            {
                // First case: the "pattern" is smaller than the remaining decimal digits.
                if (strlen(printed_value) - p_end > strlen(pattern))
                {
                    // If the pattern is found again in the remaining decimal digits, jump over it.
                    if (strncmp(pattern, printed_value + p_end, strlen(pattern)) == 0)
                    {
                        p_start += strlen(pattern);
                        p_end += strlen(pattern);
                    }
                    // If not, copy the digits between p_start and p_end to the pattern.
                    else
                    {
                        strncpy(pattern, printed_value + p_start, p_end - p_start + 1);
                        pattern[p_end - p_start + 1] = '\0';
                        ++p_end;
                    }
                    // If the pattern matches the last decimal digits, stop the execution with success.
                    if (strlen(printed_value) - p_end == strlen(pattern) &&
                        strncmp(pattern, printed_value + p_end, strlen(pattern)) == 0)
                    {
                        success = true;
                        break;
                    }
                }
                // Second case: the pattern is bigger than the remaining digits.
                else
                {
                    // Consider a success the case where the remaining digits except the last one match the leftmost digits of the pattern.
                    if (strncmp(pattern, printed_value + p_end, strlen(printed_value) - p_end - 1) == 0)
                    {
                        success = true;
                        break;
                    }
                    else
                        break;
                }
            }
            if (success == true)
                break;
        }
    }
    // Simple cases with finite decimal digits
    else
    {
        result.c = pow(10, decimal_length);
        result.b = round(value * result.c);
        reduce_fraction(&result);
        return result;
    }
    // getting the fraction from the "pattern"
    if (success == true)
    {
        int pattern_start;
        // Just in case the pattern was found to be zeors due to minor rounding (like 5.0000000000000003)
        if (pattern[0] == '0')
            return result;

        // Generate the denominator
        for (p_start = 0; p_start < strlen(pattern); ++p_start)
            result.c += 9 * pow(10, p_start);
        // Find the pattern start in case it doesn't start right after the decimal point (like 0.79999)
        pattern_start = f_search(printed_value, pattern, decimal_point + 1, false);
        if (pattern_start > decimal_point + 1)
        {
            result.b = round(value * (pow(10, pattern_start - decimal_point - 1 + strlen(pattern)) - pow(10, pattern_start - decimal_point - 1)));
            result.c *= pow(10, pattern_start - decimal_point - 1);
        }
        else
            sscanf(pattern, "%d", &result.b);
        reduce_fraction(&result);
        return result;
    }
    // trying to get the fraction by inverting it then trying the algorithm again
    else if (inverse_process == false)
    {
        double inverse_value = 1 / value;
        // case where the fraction is likely of form 1/x
        if (fabs(inverse_value - floor(inverse_value)) < 1e-10)
        {
            result.b = 1;
            result.c = inverse_value;
            return result;
        }
        // Other cases like 3/17 (non periodic but the inverse is periodic)
        else
        {
            fraction inverted = decimal_to_fraction(inverse_value, true);
            if (inverted.c != 0)
            {
                // inverse of a + b / c is c / ( a *c + b )
                result.b = inverted.c;
                result.c = inverted.b + inverted.a * inverted.c;
            }
        }
    }
    return result;
}
void priority_fill(op_node *list, int op_count)
{
    char operators[6] = {'^', '*', '/', '%', '+', '-'};
    uint8_t priority[6] = {3, 2, 2, 2, 1, 1};
    int i, j;
    for (i = 0; i < op_count; ++i)
    {
        for (j = 0; j < 7; ++j)
        {
            if (list[i].operator== operators[j])
            {
                list[i].priority = priority[j];
                break;
            }
        }
    }
}

int find_subexpr_by_start(m_subexpr *S, int start, int s_index, int mode)
{
    int i;
    i = s_index - 1;
    switch (mode)
    {
    case 1:
        while (i >= 0)
        {
            if (S[i].subexpr_start == start)
                return i;
            else
                --i;
        }
        break;
    case 2:
        while (i >= 0)
        {
            if (S[i].solve_start == start)
                return i;
            else
                --i;
        }
    }
    return -1;
}
// Function that finds the subexpression that ends at 'end'
int find_subexpr_by_end(m_subexpr *S, int end, int s_index, int s_count)
{
    int i;
    i = s_index - 1;
    while (i < s_count)
    {
        if (S[i].solve_end == end)
            return i;
        else
            --i;
    }
    return -1;
}

void dump_expr_data(math_expr *M)
{
    int s_index = 0;
    int s_count = M->subexpr_count;
    m_subexpr *S = M->subexpr_ptr;
    op_node *tmp_node;
    puts("Dumping expression data:\n");
    while (s_index < s_count)
    {
        printf("subexpr %d:\n s_function = %p, c_function = %p, e_function = %p, depth = %d\n",
               s_index, S[s_index].function_ptr, S[s_index].cmplx_function_ptr, S[s_index].ext_function_ptr, S[s_index].depth);
        // Lookup result pointer location
        for (int i = 0; i < s_count; ++i)
        {
            for (int j = 0; j < S[i].op_count; ++j)
            {
                if (S[i].subexpr_nodes[j].node_result == *(S[s_index].s_result))
                {
                    printf("s_result at subexpr %d, node %d\n\n", i, j);
                    break;
                }
            }
            // Case of a no op expression
            if (S[i].op_count == 0 && S[i].subexpr_nodes[0].node_result == *(S[s_index].s_result))
                printf("s_result at subexpr %d, node 0\n\n", i);
        }
        tmp_node = S[s_index].subexpr_nodes + S[s_index].start_node;
        // Dump nodes data
        while (tmp_node != NULL)
        {

            printf("%d: ", tmp_node->node_index);
            printf("( ");
            print_value(tmp_node->left_operand);
            printf(" )");
            printf(" %c ", tmp_node->operator);
            printf("( ");
            print_value(tmp_node->right_operand);
            printf(" ) -> ");
            tmp_node = tmp_node->next;
        }
        ++s_index;
        puts("end\n");
    }
}