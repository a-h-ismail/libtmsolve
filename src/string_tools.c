/*
Copyright (C) 2021-2024 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "string_tools.h"
#include "scientific.h"
#include "internals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

char *tms_strndup(const char *source, size_t n)
{
    char *new = malloc((n + 1) * sizeof(char));
    strncpy(new, source, n);
    new[n] = '\0';
    return new;
}

int tms_find_closing_parenthesis(char *expr, int i)
{
    // Initializing pcount to 1 because the function receives the index of an open parenthesis
    int pcount = 1;
    while (*(expr + i) != '\0' && pcount != 0)
    {
        // Skipping over the first parenthesis
        ++i;
        if (*(expr + i) == '(')
            ++pcount;
        else if (*(expr + i) == ')')
            --pcount;
    }
    // Case where the open parenthesis has no closing one, return -1 to the calling function
    if (pcount != 0)
        return -1;
    else
        return i;
}
int tms_find_opening_parenthesis(char *expr, int p)
{
    // initializing pcount to 1 because the function receives the index of a closed parenthesis
    int pcount = 1;
    while (p != 0 && pcount != 0)
    {
        // Skipping over the first parenthesis
        --p;
        if (*(expr + p) == ')')
            ++pcount;
        else if (*(expr + p) == '(')
            --pcount;
    }
    // Case where the closed parenthesis has no opening one, return -1 to the calling function
    if (pcount != 0)
        return -1;
    else
        return p;
}

// Function to find the minimum
int find_min(int a, int b)
{
    if (a < b)
        return a;
    else
        return b;
}

double complex _tms_set_operand_value(char *expr, int start, bool enable_complex)
{
    double complex value = NAN;
    bool is_negative = false;

    // Avoid negative offsets
    if (start < 0)
        return NAN;

    // Catch incorrect start like )5 (no implied multiplication allowed)
    if (start > 0 && !tms_is_valid_number_start(expr[start - 1]))
    {
        tms_error_handler(EH_SAVE, SYNTAX_ERROR, EH_FATAL_ERROR, expr, start - 1);
        return NAN;
    }

    if (expr[start] == '-')
    {
        is_negative = true;
        ++start;
    }
    else
        is_negative = false;

    value = tms_read_value(expr, start);

    // Failed to read value normally, it is probably a variable
    if (isnan(creal(value)))
    {
        char *name = tms_get_name(expr, start, true);
        if (name == NULL)
            return NAN;

        int i = tms_find_str_in_array(name, tms_g_vars, tms_g_var_count, TMS_V_DOUBLE);
        if (i != -1)
            value = tms_g_vars[i].value;
        // ans is a special case
        else if (strcmp(name, "ans") == 0)
            value = tms_g_ans;

        free(name);
    }

    if (!enable_complex && cimag(value) != 0)
    {
        tms_error_handler(EH_SAVE, COMPLEX_DISABLED, EH_FATAL_ERROR, expr, start);
        return NAN;
    }

    if (is_negative)
        value = -value;
    return value;
}

bool tms_is_op(char c)
{
    char ops[] = {'+', '-', '*', '/', '^', '%', '='};
    for (int i = 0; i < array_length(ops); ++i)
        if (c == ops[i])
            return true;

    return false;
}

bool tms_is_int_op(char c)
{
    char ops[] = {'+', '-', '*', '/', '^', '|', '&', '%', '='};
    for (int i = 0; i < array_length(ops); ++i)
        if (c == ops[i])
            return true;

    return false;
}

bool tms_is_valid_number_start(char c)
{
    switch (c)
    {
    case '(':
    case ',':
        return true;
    default:
        return tms_is_op(c);
    }
    return false;
}

bool tms_is_valid_int_number_start(char c)
{
    switch (c)
    {
    case '(':
    case ',':
        return true;
    default:
        return tms_is_int_op(c);
    }
    return false;
}

bool tms_is_valid_number_end(char c)
{
    switch (c)
    {
    case ')':
    case ',':
    case '\0':
        return true;
    default:
        return tms_is_op(c);
    }
    return false;
}

bool tms_is_valid_int_number_end(char c)
{
    switch (c)
    {
    case ')':
    case ',':
    case '\0':
        return true;
    default:
        return tms_is_int_op(c);
    }
    return false;
}

int8_t tms_bin_to_int(char c)
{
    if (c == '0')
        return 0;
    else if (c == '1')
        return 1;
    else
        return -1;
}

int8_t tms_dec_to_int(char c)
{
    if (isdigit(c) != 0)
        return c - '0';
    else
        return -1;
}

int8_t tms_oct_to_int(char c)
{
    if (c >= '0' && c <= '7')
        return c - '0';
    else
        return -1;
}

int8_t tms_hex_to_int(char c)
{
    if (isdigit(c) != 0)
        return c - '0';
    else
    {
        c = tolower(c);
        if (c >= 'a' && c <= 'f')
            return c - 'a' + 10;
    }
    return -1;
}

int8_t tms_detect_base(char *_s)
{
    if (_s[0] == '+' || _s[0] == '-')
        ++_s;

    // Detect the number base
    if (_s[0] == '0')
    {
        switch (_s[1])
        {
        case 'x':
            return 16;
        case 'o':
            return 8;
        case 'b':
            return 2;
        }
    }
    // Assume the base is 10 if no prefix is found
    return 10;
}
double _tms_read_value_simple(char *number, int8_t base)
{
    double value = 0, power = 1;
    int i, dot = -1, start, length;
    bool is_negative = false;
    int8_t (*symbol_resolver)(char);

    // Empty string
    if (number[0] == '\0')
        return NAN;

    if (number[0] == '-')
    {
        ++number;
        is_negative = true;
    }
    else if (number[0] == '+')
        ++number;

    length = strlen(number);

    switch (base)
    {
    case 10:
        symbol_resolver = tms_dec_to_int;
        break;
    case 2:
        symbol_resolver = tms_bin_to_int;
        break;
    case 8:
        symbol_resolver = tms_oct_to_int;
        break;
    case 16:
        symbol_resolver = tms_hex_to_int;
        break;
    default:
        return NAN;
    }

    for (i = 0; i < length; ++i)
    {
        if (number[i] == '.')
        {
            dot = i;
            break;
        }
    }
    if (dot != -1)
        start = dot - 1;
    else
        start = length - 1;

    int8_t tmp;
    for (i = start; i >= 0; --i)
    {
        tmp = (*symbol_resolver)(number[i]);
        if (tmp == -1)
            return NAN;
        value += tmp * power;
        power *= base;
    }
    // Reading the decimal part
    if (dot != -1)
    {
        power = base;
        for (i = dot + 1; i < length; ++i)
        {
            value += (*symbol_resolver)(number[i]) / power;
            power *= base;
        }
    }
    if (is_negative)
        value = -value;
    return value;
}

double complex tms_read_value(char *_s, int start)
{
    double value;
    int end, sci_notation = -1, base;
    bool is_complex = false;
    _s += start;
    end = tms_find_endofnumber(_s, 0);
    if (end == -1)
        return NAN;

    if (!tms_is_valid_number_end(_s[end + 1]))
    {
        tms_error_handler(EH_SAVE, SYNTAX_ERROR, EH_FATAL_ERROR, _s, start + end);
        return NAN;
    }

    base = tms_detect_base(_s);
    if (base != 10)
    {
        _s += 2;
        end -= 2;
    }

    if (end < 0)
    {
        tms_error_handler(EH_SAVE, SYNTAX_ERROR, EH_FATAL_ERROR, _s, start);
        return NAN;
    }

    // Copy the value to a separate array
    char num_str[end + 2];
    strncpy(num_str, _s, end + 1);
    num_str[end + 1] = '\0';

    if (num_str[end] == 'i')
    {
        is_complex = true;
        num_str[end] = '\0';
    }

    // Check for scientific notation 'e' or 'E'
    if (base == 10)
        for (int i = 0; num_str[i] != '\0'; ++i)
        {
            if (num_str[i] == 'e' || num_str[i] == 'E')
            {
                sci_notation = i;
                break;
            }
        }

    // No scientific notation found (or prevented for non base 10 representations)
    if (sci_notation == -1)
    {
        value = _tms_read_value_simple(num_str, base);
        if (is_complex)
            return value * I;
        else
            return value;
    }
    else
    {
        double power;
        num_str[sci_notation] = '\0';
        value = _tms_read_value_simple(num_str, 10);
        power = _tms_read_value_simple(num_str + sci_notation + 1, 10);
        if (is_complex)
            return value * pow(10, power) * I;
        else
            return value * pow(10, power);
    }
}

int64_t _tms_read_int_helper(char *number, int8_t base)
{
    int64_t value = 0;
    int i, power = 1;
    bool is_negative = false;
    int8_t (*symbol_resolver)(char);

    // Empty string
    if (number[0] == '\0')
    {
        tms_error_bit = 1;
        return -1;
    }

    if (number[0] == '-')
    {
        ++number;
        is_negative = true;
    }
    else if (number[0] == '+')
        ++number;

    switch (base)
    {
    case 10:
        symbol_resolver = tms_dec_to_int;
        break;
    case 2:
        symbol_resolver = tms_bin_to_int;
        break;
    case 8:
        symbol_resolver = tms_oct_to_int;
        break;
    case 16:
        symbol_resolver = tms_hex_to_int;
        break;
    default:
        tms_error_bit = 1;
        return -1;
    }

    int8_t tmp;
    for (i = strlen(number) - 1; i >= 0; --i)
    {
        tmp = (*symbol_resolver)(number[i]);
        if (tmp == -1)
        {
            tms_error_bit = 1;
            return -1;
        }
        value += tmp * power;
        power *= base;
    }
    if (is_negative)
        value = -value;
    return value;
}

int64_t tms_read_int_value(char *_s, int start)
{
    int64_t value;
    int end, base;

    _s += start;
    end = tms_find_int_endofnumber(_s, 0);
    if (end == -1)
    {
        tms_error_bit = 1;
        return -1;
    }

    if (!tms_is_valid_int_number_end(_s[end + 1]))
    {
        tms_error_handler(EH_SAVE, SYNTAX_ERROR, EH_FATAL_ERROR, _s, start + end);
        tms_error_bit = 1;
        return -1;
    }

    base = tms_detect_base(_s);
    if (base != 10)
    {
        _s += 2;
        end -= 2;
    }

    if (end < 0)
    {
        tms_error_handler(EH_SAVE, SYNTAX_ERROR, EH_FATAL_ERROR, _s, start);
        tms_error_bit = 1;
        return -1;
    }

    // Copy the value to a separate array
    char num_str[end + 2];
    strncpy(num_str, _s, end + 1);
    num_str[end + 1] = '\0';

    value = _tms_read_int_helper(num_str, base);
    return value;
}

// Function that seeks for the next occurence of a + or - sign starting from i
int tms_find_add_subtract(char *expr, int i)
{
    while (expr[i] != '\0')
    {
        if (expr[i] == '+' || expr[i] == '-')
            return i;
        else
            ++i;
    }
    return -1;
}
// Function that returns the index of the next operator starting from i
// if no operators are found, returns -1.
int tms_next_op(char *expr, int i)
{
    while (*(expr + i) != '\0')
    {
        if (tms_is_op(expr[i]) == false)
            ++i;
        else
            break;
    }
    if (*(expr + i) != '\0')
        return i;
    else
        return -1;
}
/*Function that combines the add/subtract operators (ex: -- => +) in the area of expr limited by a,b.
Returns true on success and false in case of invalid indexes a and b.
*/
void tms_combine_add_sub(char *expr, int a, int b)
{
    int subcount, i, j;
    if (b > strlen(expr) || a > b || a < 0)
        return;
    i = tms_find_add_subtract(expr, a);
    while (i != -1 && i < b)
    {
        j = i;
        subcount = 0;
        if (expr[j] == '-')
            ++subcount;
        while (1)
        {
            if (expr[j + 1] == '-')
                ++subcount;
            else if (expr[j + 1] != '+')
                break;
            ++j;
        }
        if (subcount % 2 == 1)
            expr[i] = '-';
        else
            expr[i] = '+';
        if (i < j)
            tms_resize_zone(expr, j, i);
        i = tms_find_add_subtract(expr, i + 1);
    }
}

void tms_remove_whitespace(char *str)
{
    int start, end, length;
    length = strlen(str);
    for (start = 0; start < length; ++start)
    {
        if (isspace(str[start]))
        {
            end = start;
            while (isspace(str[end]))
                ++end;
            memmove(str + start, str + end, length - end + 1);
            length -= end - start;
        }
    }
}

void tms_resize_zone(char *str, int old_end, int new_end)
{
    // case when size remains the same
    if (old_end == new_end)
        return;
    // Other cases
    else
        memmove(str + new_end + 1, str + old_end + 1, strlen(str + old_end));
}
bool tms_valid_digit_for_base(char digit, int8_t base)
{
    switch (base)
    {
    case 10:
        if (isdigit(digit) != 0)
            return true;
        break;
    case 16:
        if (tms_hex_to_int(digit) != -1)
            return true;
        break;
    case 8:
        if (tms_oct_to_int(digit) != -1)
            return true;
        break;
    case 2:
        if (tms_bin_to_int(digit) != -1)
            return true;
        break;
    }
    return false;
}
// [\+-]?\d+(\.\d+)?([eE]?[\+-]*\d)?+i? (May use it one day to detect numbers)

int tms_find_endofnumber(char *number, int start)
{
    int end = start, remaining_dots = 1, base = 10;
    bool is_scientific = false, is_complex = false;

    // Number starts with + or -
    if (number[start] == '+' || number[start] == '-')
        ++end;

    base = tms_detect_base(number + end);
    if (base != 10)
        end += 2;

    while (number[end] != '\0')
    {
        if (tms_valid_digit_for_base(number[end], base))
            ++end;
        else if (number[end] == '.')
        {
            if (remaining_dots == 0)
            {
                tms_error_handler(EH_SAVE, SYNTAX_ERROR, EH_FATAL_ERROR, number, end);
                return -1;
            }
            else
            {
                ++end;
                --remaining_dots;
            }
        }
        else if (base == 10 && (number[end] == 'e' || number[end] == 'E'))
        {
            // Found e or E earlier, doesn't happen in a number
            if (is_scientific)
                return 0;

            is_scientific = true;
            // Exponential notation may have a dot
            ++remaining_dots;
            ++end;

            // For cases such as 1e+1 or 1e-1
            if (number[end] == '+' || number[end] == '-')
                ++end;
        }
        else if (number[end] == 'i')
        {
            if (is_complex)
                return -1;

            is_complex = true;
            ++end;
        }
        else
            break;
    }

    if (number[end] == '\0' || tms_is_op(number[end]) || number[end] == ')' || number[end] == ',')
        return end - 1;
    else
        return -1;
}

int tms_find_int_endofnumber(char *number, int start)
{
    int end = start, base = 10;

    // Number starts with + or -
    if (number[start] == '+' || number[start] == '-')
        ++end;

    base = tms_detect_base(number + end);
    if (base != 10)
        end += 2;

    while (number[end] != '\0')
    {
        if (tms_valid_digit_for_base(number[end], base))
            ++end;
        else
            break;
    }

    if (number[end] == '\0' || tms_is_int_op(number[end]) || number[end] == ')' || number[end] == ',')
        return end - 1;
    else
        return -1;
}

int tms_find_startofnumber(char *expr, int end)
{
    int start = end;
    /*
    Algorithm:
    * Starting from start=end:
    * If start==0, break.
    * Check if the char at start-1 is a number, if true decrement start.
    * If not handle the following cases:
    * expr[start-1] is the imaginary number 'i': decrement start
    * expr[start-1] is a point, scientific notation (e,E): decrement start
    * expr[start-1] is an add/subtract following a scientific notation: subtract 2 from start
    * expr[start-1] is a - operator: decrement start then break
    *
    * If none of the conditions above applies, break.
    */
    while (1)
    {
        if (start == 0)
            break;
        if (isdigit(expr[start - 1]))
            --start;
        else
        {
            if (expr[start - 1] == 'i')
            {
                --start;
            }
            if (expr[start - 1] == 'e' || expr[start - 1] == 'E' || expr[start - 1] == '.')
                --start;
            else if (start > 1 && (expr[start - 1] == '+' || expr[start - 1] == '-') && (expr[start - 2] == 'e' || expr[start - 2] == 'E'))
                start -= 2;
            else if (expr[start - 1] == '-')
            {
                --start;
                break;
            }
            else
                break;
        }
    }
    return start;
}

int tms_f_search(char *str, char *keyword, int index, bool match_word)
{
    char *match;
    // Search for the needle in the haystack
    match = strstr(str + index, keyword);
    if (match == NULL)
        return -1;
    else
    {
        if (match_word)
        {
            int keylen = strlen(keyword);
            do
            {
                // Matched at the beginning
                if (match == str && !tms_legal_char_in_name(match[keylen]))
                    return match - str;
                // Matched elswhere
                else if (match != str && !tms_legal_char_in_name(match[-1]) && !tms_legal_char_in_name(match[keylen]))
                    return match - str;
                else
                    match = strstr(match + keylen, keyword);
            } while (match != NULL);
            return -1;
        }
        else
            return match - str;
    }
}

// Reverse search function, returns the index of the first match of keyword, or -1 if none is found
// adjacent_search stops the search if the index is not inside the keyword
int tms_r_search(char *str, char *keyword, int index, bool adjacent_search)
{
    int length = strlen(keyword), i = index;
    while (i > -1)
    {
        if (adjacent_search == true && i == index - length)
            return -1;
        if (str[i] == keyword[0])
        {
            if (strncmp(str + i, keyword, length) == 0)
                return i;
            else
                --i;
        }
        else
            --i;
    }
    return -1;
}

bool tms_match_word(char *str, int i, char *keyword, bool match_from_start)
{
    int keylen = strlen(keyword);
    if (!match_from_start)
        i -= keylen - 1;

    if (i < 0)
        return false;

    if (strncmp(str + i, keyword, keylen) != 0)
        return false;

    if (i > 0 && tms_legal_char_in_name(str[i - 1]))
        return false;

    if (tms_legal_char_in_name(str[i + keylen]))
        return false;

    return true;
}

void tms_print_value(double complex value)
{
    if (creal(value) != 0)
    {
        printf("%g", creal(value));
        if (cimag(value) != 0)
        {
            if (cimag(value) > 0)
                printf(" + %g * i", cimag(value));
            else
                printf(" - %g * i", fabs(cimag(value)));
        }
    }
    else if (cimag(value) != 0)
    {
        if (cimag(value) > 0)
            printf("%g * i", cimag(value));
        else
            printf("- %g * i", fabs(cimag(value)));
    }
    else
        printf("0");
}

// Function that checks the existence of value in an int array of "count" ints
bool int_search(int *array, int value, int count)
{
    int i;
    for (i = 0; i < count; ++i)
    {
        if (array[i] == value)
            return true;
    }
    return false;
}

bool tms_parenthesis_check(char *expr)
{
    int open, close = -1, k = 0, *open_position, *close_position, length = strlen(expr);

    if (length == 0)
    {
        tms_error_handler(EH_SAVE, NO_INPUT, EH_FATAL_ERROR, NULL);
        return false;
    }

    open_position = (int *)malloc(length * sizeof(int));
    close_position = (int *)malloc(length * sizeof(int));
    *open_position = *close_position = -2;
    open = tms_f_search(expr, "(", 0, false);
    // Check if every open parenthesis has a close parenthesis and log their indexes
    while (open != -1)
    {
        close = tms_find_closing_parenthesis(expr, open);
        if (close == -1)
        {
            tms_error_handler(EH_SAVE, PARENTHESIS_NOT_CLOSED, EH_FATAL_ERROR, expr, open);
            free(open_position);
            free(close_position);
            return false;
        }
        open_position[k] = open;
        close_position[k] = close;
        open = tms_f_search(expr, "(", open + 1, false);
        ++k;
    }
    qsort(open_position, k, sizeof(int), compare_ints);
    qsort(close_position, k, sizeof(int), compare_ints_reverse);
    // Case of no open parenthesis, check if a close parenthesis is present
    if (k == 0)
        close = tms_f_search(expr, ")", 0, false);
    else
        close = tms_f_search(expr, ")", close_position[0] + 1, false);

    if (close != -1)
    {
        tms_error_handler(EH_SAVE, PARENTHESIS_NOT_OPEN, EH_FATAL_ERROR, expr, close);
        free(open_position);
        free(close_position);
        return false;
    }
    // Checking that no useless parenthesis pairs are written (planned for later)
    free(open_position);
    free(close_position);
    return true;
}

bool tms_syntax_check(char *expr)
{
    int i, j, length;

    // Check if all function calls have parenthesis
    for (i = 0; i < tms_g_func_count; ++i)
    {
        length = strlen(tms_g_all_func_names[i]);
        j = tms_f_search(expr, tms_g_all_func_names[i], 0, true);
        while (j != -1)
        {
            if (expr[j + length] != '(')
            {
                tms_error_handler(EH_SAVE, PARENTHESIS_MISSING, EH_FATAL_ERROR, expr, j + strlen(tms_g_all_func_names[i]));
                return false;
            }
            j = tms_f_search(expr, tms_g_all_func_names[i], j + 1, true);
        }
    }
    return tms_parenthesis_check(expr);
}

int tms_compare_priority(char operator1, char operator2)
{
    char operators[7] = {'!', '^', '*', '/', '%', '+', '-'};
    short priority[7] = {4, 3, 2, 2, 2, 1, 1}, op1_p = '\0', op2_p = '\0', i;

    for (i = 0; i < 7; ++i)
    {
        if (operator1 == operators[i])
        {
            op1_p = priority[i];
            break;
        }
        if (operator2 == operators[i])
        {
            op2_p = priority[i];
            break;
        }
    }
    if (op1_p == '\0' || op2_p == '\0')
    {
        puts("Invalid operators in priority_test.");
        exit(3);
    }
    return op1_p - op2_p;
}

tms_arg_list *tms_get_args(char *string)
{
    tms_arg_list *A = malloc(sizeof(tms_arg_list));
    A->count = 0;

    int length = strlen(string), max_args = 10;
    // The start/end of each argument.
    int start, end;

    if (length == 0)
    {
        A->arguments = NULL;
        return A;
    }

    A->arguments = malloc(max_args * sizeof(char *));
    for (end = start = 0; end < length; ++end)
    {
        if (A->count == max_args)
        {
            max_args += 10;
            A->arguments = realloc(A->arguments, max_args * sizeof(char *));
        }
        // Skip parenthesis pairs to allow easy nesting of argument lists
        // Think of something like int(0,2,x+int(0,1,x^2))
        if (string[end] == '(')
            end = tms_find_closing_parenthesis(string, end);
        else if (string[end] == ',')
        {
            A->arguments[A->count] = malloc(end - start + 1);
            strncpy(A->arguments[A->count], string + start, end - start);
            A->arguments[A->count][end - start] = '\0';
            start = end + 1;
            ++A->count;
        }
    }
    A->arguments[A->count] = malloc(end - start + 1);
    strncpy(A->arguments[A->count], string + start, end - start);
    A->arguments[A->count][end - start] = '\0';
    ++A->count;
    return A;
}

void tms_free_arg_list(tms_arg_list *list)
{
    for (int i = 0; i < list->count; ++i)
        free(list->arguments[i]);
    free(list->arguments);
    free(list);
}

bool tms_pre_parse_routine(char *expr)
{
    // Check for empty input
    if (expr[0] == '\0')
    {
        tms_error_handler(EH_SAVE, NO_INPUT, EH_FATAL_ERROR, NULL);
        return false;
    }
    tms_remove_whitespace(expr);
    // Combine multiple add/subtract symbols (ex: -- becomes + or +++++ becomes +)
    tms_combine_add_sub(expr, 0, strlen(expr) - 2);
    return tms_syntax_check(expr);
}

bool tms_legal_char_in_name(char c)
{
    if (isalnum(c) || c == '_')
        return true;
    else
        return false;
}

bool tms_valid_name(char *name)
{
    bool only_digits = true;

    // Name shouldn't start with a number
    if (isalpha(name[0]) || name[0] == '_')
        only_digits = false;
    else
        return false;

    for (int i = 0; i < strlen(name); ++i)
    {
        if (isalpha(name[i]) || name[i] == '_')
            only_digits = false;
        else if (isdigit(name[i]))
            continue;
        else
            return false;
    }
    if (only_digits)
        return false;

    return true;
}

void tms_print_bin(int64_t value)
{
    // This bool determines if the zeros are trailing or not
    bool trailing = true;
    uint8_t digit;
    // An 64 bit int has the MSB as an octal, then every octal digit is 3 bits
    // Go left to right, read 3 bits and print the digit to stdout

    printf("0b");
    if (value == 0)
    {
        printf("0");
        return;
    }

    for (int i = 0; i < 64; ++i)
    {
        // Shift 63 positions for the MSB to become LSB
        digit = ((uint64_t)value) >> 63;

        if (digit == 0)
        {
            if (!trailing)
                putchar('0');
        }
        else
        {
            putchar('1');
            trailing = false;
        }

        value = value << 1;
    }
}

void tms_print_oct(int64_t value)
{
    // This bool determines if the zeros are trailing or not
    bool trailing = true;
    uint8_t digit;
    // An 64 bit int has the MSB as an octal, then every octal digit is 3 bits
    // Go left to right, read 3 bits and print the digit to stdout

    printf("0o");
    if (value == 0)
    {
        printf("0");
        return;
    }

    // The first bit
    if ((value & 0x8000000000000000) != 0)
    {
        trailing = false;
        putchar('1');
    }

    for (int i = 0; i < 63 / 3; ++i)
    {
        // Mask the most significant three bits and shift them to become LSB
        digit = ((uint64_t)value & 0x7000000000000000) >> 60;

        if (digit == 0)
        {
            if (!trailing)
                putchar('0');
        }
        else
        {
            putchar('0' + digit);
            trailing = false;
        }

        value = value << 3;
    }
}

char int_to_hex(int8_t v)
{
    if (v < 10)
        return '0' + v;
    else
        return 'A' + v - 10;
}

void tms_print_hex(int64_t value)
{
    // This bool determines if the zeros are trailing or not
    bool trailing = true;
    uint8_t digit;

    printf("0x");
    if (value == 0)
    {
        printf("0");
        return;
    }

    for (int i = 0; i < 16; ++i)
    {
        // Shift 4 MSB to become LSB
        digit = (uint64_t)value >> 60;

        if (digit == 0)
        {
            if (!trailing)
                putchar('0');
        }
        else
        {
            putchar(int_to_hex(digit));
            trailing = false;
        }

        value = value << 4;
    }
}

int tms_find_str_in_array(char *key, void *array, int arr_len, uint8_t type)
{
    int i;
    switch (type)
    {
    case TMS_NOFUNC:
    case TMS_F_REAL:
    case TMS_F_CMPLX:
    case TMS_F_EXTENDED:
    case TMS_F_INT64:
    case TMS_F_INT_EXTENDED:
        char **c_array = (char **)array;
        if (arr_len != -1)
        {
            for (i = 0; i < arr_len; ++i)
            {
                if (c_array[i][0] == key[0] && strcmp(key, c_array[i]) == 0)
                    return i;
            }
        }
        else
        {
            // Case where the array length is not known, keep running until we hit NULL
            for (i = 0; c_array[i] != NULL; ++i)
            {
                if (c_array[i][0] == key[0] && strcmp(key, c_array[i]) == 0)
                    return i;
            }
        }
        break;

    case TMS_F_RUNTIME:
        tms_ufunc *ufunc_array = array;
        for (i = 0; i < arr_len; ++i)
        {
            if (ufunc_array[i].name[0] == key[0] && strcmp(key, ufunc_array[i].name) == 0)
                return i;
        }
        break;

    case TMS_V_DOUBLE:
        tms_var *dvars = array;
        for (int i = 0; i < arr_len; ++i)
        {
            if (dvars[i].name[0] == key[0] && strcmp(key, dvars[i].name) == 0)
                return i;
        }
        break;

    case TMS_V_INT64:
        tms_int_var *ivars = array;
        for (i = 0; i < arr_len; ++i)
        {
            if (ivars[i].name[0] == key[0] && strcmp(key, ivars[i].name) == 0)
                return i;
        }
        break;
    }

    return -1;
}

int tms_name_bounds(char *expr, int i, bool is_at_start)
{
    if (is_at_start)
    {
        // The name can't start with a number
        if (isdigit(expr[i]))
            return -1;

        int end = i;

        while (tms_legal_char_in_name(expr[end]))
            ++end;

        // The loop immediatly stopped, so we didn't get a name
        if (i == end)
            return -1;

        // "end" overshoots before breaking from the loop, so decrement it
        --end;

        return end;
    }
    else
    {
        int start = i;

        while (start >= 0 && tms_legal_char_in_name(expr[start]))
            --start;

        // Again, the loop didn't execute even once
        if (i == start)
            return -1;

        // "start" undershoots before breaking from the loop, so increment it
        ++start;

        // Is the name really valid? (the beginning is not a digit)
        if (isdigit(expr[start]))
            return -1;
        else
            return start;
    }
}

char *tms_get_name(char *expr, int i, bool is_at_start)
{
    if (is_at_start)
    {
        int end = tms_name_bounds(expr, i, true);

        if (end == -1)
            return NULL;
        else
            return tms_strndup(expr + i, end - i + 1);
    }
    else
    {
        int start = tms_name_bounds(expr, i, false);

        if (start == -1)
            return NULL;
        else
            return tms_strndup(expr + start, i - start + 1);
    }
}

char *tms_strcat_dup(char *s1, char *s2)
{
    char *concatenated_string = malloc((strlen(s1) + strlen(s2) + 1) * sizeof(char));

    strcpy(concatenated_string, s1);
    strcpy(concatenated_string + strlen(s1), s2);

    return concatenated_string;
}
