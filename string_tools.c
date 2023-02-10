/*
Copyright (C) 2021-2023 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "string_tools.h"
#include "scientific.h"
#include "internals.h"
// Simple function to detect the word "inf", almost useless since the same can be accomplished with f_search
bool is_infinite(char *expr, int index)
{
    if (expr[index] == '+' || expr[index] == '-')
        ++index;
    if (strncmp(expr + index, "inf", 3) == 0)
        return true;
    else
        return false;
}

int find_closing_parenthesis(char *expr, int i)
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
int find_opening_parenthesis(char *expr, int p)
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
// Returns the value of the number or variable (can be constant like pi or variable like ans) starting at (expr + start)
double complex read_value(char *expr, int start, bool enable_complex)
{
    double complex value;
    bool is_negative, is_complex = false;
    int flag = 0;

    if (expr[start] == '-')
    {
        is_negative = true;
        ++start;
    }
    else
        is_negative = false;

    int i;
    for (i = 0; i < variable_count; ++i)
    {
        if (strncmp(expr + start, variable_names[i], strlen(variable_names[i])) == 0)
        {
            if (start > 0 && is_alphabetic(expr[start - 1]))
            {
                error_handler(SYNTAX_ERROR, 1, 1, start);
                return NAN;
            }
            int end = start + strlen(variable_names[i]);
            if (!(is_op(expr[end]) || expr[end] == '\0' || expr[end] == ')'))
            {
                error_handler(SYNTAX_ERROR, 1, 1, end);
                return NAN;
            }
            // ans is a special case.
            if (i == 2)
                value = ans;
            else
                value = variable_values[i];
            flag = 2;
            break;
        }
    }

    if (flag == 0)
    {
        // Check for complex number
        if (enable_complex)
        {
            int end = find_endofnumber(expr, start);
            if (expr[start] == 'i')
            {
                // Using XOR to flip the boolean from false to true
                is_complex = is_complex ^ 1;
                if (start == end)
                {
                    if (is_negative)
                        return -I;
                    else
                        return I;
                }
                ++start;
            }
            if (expr[end] == 'i')
                // Again, XOR toggles between true and false
                is_complex = is_complex ^ 1;
        }

        double tmp;
        flag = sscanf(expr + start, "%lf", &tmp);
        value = tmp;
        // Case where nothing was found even after checking for variables
        if (flag == 0)
            return NAN;
    }
    if (is_negative)
        value = -value;
    if (enable_complex && is_complex)
        return value * I;
    else
        return value;
}
bool is_op(char c)
{
    switch (c)
    {
    case '/':
        return true;
    case '*':
        return true;
    case '+':
        return true;
    case '-':
        return true;
    case '^':
        return true;
    case '!':
        return true;
    case '%':
        return true;
    case '=':
        return true;
    default:
        return false;
    }
}

// Function that seeks for the next occurence of a + or - sign starting from i
int find_add_subtract(char *expr, int i)
{
    while (expr[i] != '+' && expr[i] != '-' && expr[i] != '\0')
    {
        ++i;
        if ((expr[i - 1] == 'e' || expr[i - 1] == 'E') && expr[i] != '\0')
            ++i;
    }
    // Check that the stop was not caused by reaching \0
    if (expr[i] == '\0')
        return -1;
    else
        return i;
}
// Function that returns the index of the next operator starting from i
// if no operators are found, returns -1.
int next_op(char *expr, int i)
{
    while (*(expr + i) != '\0')
    {
        if (is_op(expr[i]) == false)
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
void combine_add_subtract(char *expr, int a, int b)
{
    int subcount, i, j;
    if (b > strlen(expr) || a > b || a < 0)
        return;
    i = find_add_subtract(expr, a);
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
            resize_zone(expr, j, i);
        i = find_add_subtract(expr, i + 1);
    }
}

void remove_whitespace(char *str)
{
    int start, end, length;
    length = strlen(str);
    for (start = 0; start < length; ++start)
    {
        if (str[start] == ' ')
        {
            end = start;
            while (str[end] == ' ')
                ++end;
            memmove(str + start, str + end, length - end + 1);
            length -= end - start;
        }
    }
}
/*
    Utility function to resize a region inside a string
    This function receives the address of the string (*str),
    the index of last element of the region (end), the new index of the last element after resize.
*/
void resize_zone(char *str, int o_end, int n_end)
{
    // case when size remains the same
    if (o_end == n_end)
        return;
    // Other cases
    else
        memmove(str + n_end + 1, str + o_end + 1, strlen(str + o_end));
}
inline bool is_digit(char c)
{
    if (c >= '0' && c <= '9')
        return true;
    else
        return false;
}
inline bool is_alphabetic(char c)
{
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
        return true;
    else
        return false;
}
inline bool is_alphanum(char c)
{
    return is_digit(c) && is_alphabetic(c);
}

// Checks if keyword1 found at index of expr is a part of keyword2
bool part_of_keyword(char *expr, char *keyword1, char *keyword2, int index)
{
    int keylen[2], i;
    keylen[0] = strlen(keyword1);
    keylen[1] = strlen(keyword2);
    // If the length of keyword1 is bigger or equal to keyword2, then no way keyword1 is part of keyword2
    if (keylen[0] >= keylen[1])
        return false;
    i = r_search(expr, keyword2, index, false);
    if (i == -1)
        return false;
    // If the distance between the match of keyword2 and keyword1 is less than the length of keyword2
    // then keyword1 is included in keyword 2
    if (index - i < keylen[1])
        return true;
    else
        return false;
}
/*
Function that converts implicit multiplication into explicit multiplication, direction 'r', 'l'
For left implicit multiplication:
p1 must be the start of the first operand, p2 the start of the second operand and p3 its end.
For right implicit multiplication:
p1 must be the start of the first operand, p2 the end of it, and p3 the end of the second operand.
*/
void shift_and_multiply(char *expr, int *p1, int *p2, int *p3, char direction)
{
    switch (direction)
    {
    case 'l':
        // Creating 3 empty spaces to accomodate for ( * )
        resize_zone(expr, *p3, *p3 + 3);
        memmove(expr + *p2 + 2, expr + *p2, *p3 - *p2 + 1);
        *p3 += 3;
        memmove(expr + *p1 + 1, expr + *p1, *p2 - *p1);
        expr[*p1] = '(';
        expr[*p2 + 1] = '*';
        expr[*p3] = ')';
        *p2 = *p1;
        break;
    case 'r':
        // Creating 3 empty spaces to accomodate for ( * )
        resize_zone(expr, *p3, *p3 + 3);
        memmove(expr + *p2 + 3, expr + *p2 + 1, *p3 - *p2);
        *p3 += 2;
        memmove(expr + *p1 + 1, expr + *p1, *p2 - *p1 + 1);
        expr[*p1] = '(';
        expr[*p3 + 1] = ')';
        expr[*p2 + 2] = '*';
        ++*p1;
    }
}
// Function that checks if the number starting at start is valid
bool is_valid_number(char *expr, int start)
{
    if (expr[start] == '+' || expr[start] == '-')
        ++start;
    if (is_digit(expr[start]) || expr[start] == 'i')
        return true;
    else
        return false;
}

int find_endofnumber(char *expr, int start)
{
    int end = start;
    /*
    Algorithm:
    * Starting from "start" check if the char at end+1 is a number, if true increment end
    * If not, check the following cases:
    * expr[end+1] is a point '.' before scientific notation, scientific notations 'e' 'E': increment end
    * expr[end+1] is the imaginary number 'i': Increment end then break
    * expr[end+1] is an add/subtract operator following a scientific notation, increment end
    *
    * If none of the following cases applies, break execution and return end
    */
    while (1)
    {
        if (is_digit(expr[end + 1]) == true && expr[end + 1] != '\0')
            ++end;
        else
        {
            if (expr[end + 1] == 'e' || expr[end + 1] == 'E')
                ++end;
            else if (expr[end + 1] == '.')
                ++end;
            else if ((expr[end + 1] == '+' || expr[end + 1] == '-') && (expr[end] == 'e' || expr[end] == 'E'))
                ++end;
            else if (expr[end + 1] == 'i')
            {
                ++end;
                break;
            }
            else
                break;
        }
    }
    return end;
}

int find_startofnumber(char *expr, int end)
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
        if (is_digit(expr[start - 1]) == true)
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

int f_search(char *str, char *keyword, int index, bool match_word)
{
    int keylen = strlen(keyword), s_length = strlen(str);
    while (index < s_length)
    {
        if (str[index] == keyword[0])
        {
            if (strncmp(str + index, keyword, keylen) == 0)
            {
                // match_word: keyword match isn't adjacent to alphabetic or underscore characters.
                if (match_word)
                {
                    
                    if ((index > 0 && (is_alphabetic(str[index - 1]) == true || str[index - 1] == '_')) ||
                        ((is_alphabetic(str[index + keylen]) == true || str[index + keylen] != '_')))
                    {
                        index += keylen;
                        continue;
                    }
                }
                return index;
            }
            else
                index += keylen;
        }
        else
            ++index;
    }
    return -1;
}
// Reverse search function, returns the index of the first match of keyword, or -1 if none is found
// adjacent_search stops the search if the index is not inside the keyword
int r_search(char *str, char *keyword, int index, bool adjacent_search)
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

void print_value(double complex value)
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

/*
Function that checks if every open parenthesis has a closing parenthesis and that no parenthesis pair is empty.
Returns true when checks pass
*/
bool parenthesis_check(char *expr)
{
    int open, close = -1, k = 0, *open_position, *close_position, length = strlen(expr);
    open_position = (int *)malloc(length * sizeof(int));
    close_position = (int *)malloc(length * sizeof(int));
    *open_position = *close_position = -2;
    open = f_search(expr, "(", 0, false);
    // Check if every open parenthesis has a close parenthesis and log their indexes
    while (open != -1)
    {
        close = find_closing_parenthesis(expr, open);
        if (close - open == 1)
        {
            error_handler(PARENTHESIS_EMPTY, 1, 1, open);
            free(open_position);
            free(close_position);
            return false;
        }
        if (close == -1)
        {
            error_handler(PARENTHESIS_NOT_CLOSED, 1, 1, open);
            free(open_position);
            free(close_position);
            return false;
        }
        open_position[k] = open;
        close_position[k] = close;
        open = f_search(expr, "(", open + 1, false);
        ++k;
    }
    qsort(open_position, k, sizeof(int), compare_ints);
    qsort(close_position, k, sizeof(int), compare_ints_reverse);
    // Case of no open parenthesis, check if a close parenthesis is present
    if (k == 0)
        close = f_search(expr, ")", 0, false);
    else
        close = f_search(expr, ")", close_position[0] + 1, false);

    if (close != -1)
    {
        error_handler(PARENTHESIS_NOT_OPEN, 1, 1, close);
        free(open_position);
        free(close_position);
        return false;
    }
    // Checking that no useless parenthesis pairs are written (planned for later)
    free(open_position);
    free(close_position);
    return true;
}

bool syntax_check(char *expr)
{
    int length = strlen(expr), i, j;
    tmsolve_init();
    _glob_expr = expr;
    // First check: all function calls have parenthesis.
    for (i = 0; i < function_count; ++i)
    {
        j = f_search(expr, all_functions[i], 0, true);
        while (j != -1)
        {
            if (expr[j + strlen(all_functions[i])] != '(')
            {
                error_handler(PARENTHESIS_MISSING, 1, 1, j + strlen(all_functions[i]));
                return false;
            }
            j = f_search(expr, all_functions[i], j + 1, true);
        }
    }
    // Check for incorrect syntax or implied multiplication.
    for (i = 0; i < length; ++i)
    {
        if (is_digit(expr[i]))
        {
            int end = find_endofnumber(expr, i);
            if (i > 0 && !is_op(expr[i - 1]) && expr[i - 1] != '(' && expr[i - 1] != ',')
            {
                error_handler(SYNTAX_ERROR, 1, 1, i - 1);
                return false;
            }
            if (expr[end + 1] != '\0' && !is_op(expr[end + 1]) && expr[end + 1] != ')' && expr[end + 1] != ',')
            {
                error_handler(SYNTAX_ERROR, 1, 1, end + 1);
                return false;
            }
            i = end + 1;
        }
    }
    return parenthesis_check(expr);
}

int compare_priority(char operator1, char operator2)
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

arg_list *get_arguments(char *string)
{
    arg_list *args = malloc(sizeof(arg_list));
    int length = strlen(string), max_args = 10, count;
    // The start/end of each argument.
    int start, end;
    // You could use args.arg_count but this improves readability (the compiler should optimize this).
    count = 0;
    args->arguments = malloc(max_args * sizeof(char *));
    for (end = start = 0; end < length; ++end)
    {
        if (count == max_args)
        {
            max_args += 10;
            args = realloc(args, max_args * sizeof(arg_list));
        }
        if (string[end] == '(')
            end = find_closing_parenthesis(string, end) + 1;
        else if (string[end] == ',')
        {
            args->arguments[count] = malloc(end - start + 1);
            strncpy(args->arguments[count], string + start, end - start);
            args->arguments[count][end - start] = '\0';
            start = end + 1;
            ++count;
        }
    }
    args->arguments[count] = malloc(end - start + 1);
    strncpy(args->arguments[count], string + start, end - start);
    args->arguments[count][end - start] = '\0';
    ++count;
    args->arg_count = count;
    return args;
}
// Frees the argument list array of char *
// Can also free the list itself if it was allocated with malloc
void free_arg_list(arg_list *list, bool list_on_heap)
{
    for (int i = 0; i < list->arg_count; ++i)
        free(list->arguments[i]);
    free(list->arguments);
    if (list_on_heap)
        free(list);
}

bool pre_parse_routine(char *expr)
{
    // Check for empty input
    if (expr[0] == '\0')
    {
        error_handler(NO_INPUT, 1, 1, -1);
        return false;
    }
    // Combine multiple add/subtract symbols (ex: -- becomes + or +++++ becomes +)
    combine_add_subtract(expr, 0, strlen(expr) - 2);
    if (syntax_check(expr) == false)
        return false;
    
    return true;
}