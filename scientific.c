/*
Copyright (C) 2021-2023 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "scientific.h"
#include "internals.h"
#include "function.h"
#include "string_tools.h"


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
    
        M = parse_expr(expr, false, false);
        result = eval_math_expr(M);
        delete_math_expr(M);
        return result;
    
    case 1:
        M = parse_expr(expr, false, false);
        result = eval_math_expr(M);
        if (isnan(creal(result)))
        {
            // Check if the errors are fatal (like malformed syntax, division by zero...)
            int fatal = error_handler(NULL, 5, 1);
            if (fatal > 0)
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


double complex eval_math_expr(math_expr *M)
{
    // No NULL pointer dereference allowed.
    if (M == NULL)
        return NAN;

    op_node *i_node;
    int s_index = 0, s_count = M->subexpr_count;
    // Subexpression pointer to access the subexpression array.
    m_subexpr *S = M->subexpr_ptr;
    // dump_expr_data(M);
    while (s_index < s_count)
    {
        // Case with special function
        if (S[s_index].nodes == NULL)
        {
            char *args;
            if (S[s_index].exec_extf)
            {
                int length = S[s_index].solve_end - S[s_index].solve_start + 1;
                // Copy args from the expression to a separate array.
                args = malloc((length + 1) * sizeof(char));
                memcpy(args, _glob_expr + S[s_index].solve_start, length * sizeof(char));
                args[length] = '\0';
                // Calling the extended function
                **(S[s_index].s_result) = (*(S[s_index].func.extended))(args);
                if (isnan((double)**(S[s_index].s_result)))
                {
                    error_handler(MATH_ERROR, 1, 0, S[s_index].solve_start, -1);
                    return NAN;
                }
                S[s_index].exec_extf = false;
                free(args);
            }
            ++s_index;

            continue;
        }
        i_node = S[s_index].nodes + S[s_index].start_node;

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
        switch (S[s_index].func_type)
        {
        case 1:
            **(S[s_index].s_result) = (*(S[s_index].func.real))(**(S[s_index].s_result));
            break;
        case 2:
            **(S[s_index].s_result) = (*(S[s_index].func.cmplx))(**(S[s_index].s_result));
            break;
        }

        if (isnan((double)**(S[s_index].s_result)))
        {
            error_handler(MATH_ERROR, 1, 0, S[s_index].solve_start, -1);
            return NAN;
        }
        ++s_index;
    }

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


void delete_math_expr(math_expr *M)
{
    if (M == NULL)
        return;

    int i = 0;
    m_subexpr *subexps = M->subexpr_ptr;
    for (i = 0; i < M->subexpr_count; ++i)
    {
        if (subexps[i].func_type == 3)
            free(subexps[i].s_result);
        free(subexps[i].nodes);
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
        printf("subexpr %d:\n function_type = %u s_function = %p, depth = %d\n",
               s_index, S[s_index].func_type, S[s_index].func.real, S[s_index].depth);
        // Lookup result pointer location
        for (int i = 0; i < s_count; ++i)
        {
            for (int j = 0; j < S[i].op_count; ++j)
            {
                if (S[i].nodes[j].node_result == *(S[s_index].s_result))
                {
                    printf("s_result at subexpr %d, node %d\n\n", i, j);
                    break;
                }
            }
            // Case of a no op expression
            if (S[i].op_count == 0 && S[i].nodes[0].node_result == *(S[s_index].s_result))
                printf("s_result at subexpr %d, node 0\n\n", i);
        }
        tmp_node = S[s_index].nodes + S[s_index].start_node;
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