/*
Copyright (C) 2021-2022 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "string_tools.h"
#include "scientific.h"
// Simple function to detect the word "inf", almost useless since the same can be accomplished with s_search
bool is_infinite(char *expr, int index)
{
    if (expr[index] == '+' || expr[index] == '-')
        ++index;
    if (strncmp(expr + index, "inf", 3) == 0)
        return true;
    else
        return false;
}
// Function to find the closed parenthesis corresponding to a specified open one
int find_closing_parenthesis(char *expr, int p)
{
    // initializing pcount to 1 because the function receives the index of an open parenthesis
    int pcount = 1;
    while (*(expr + p) != '\0' && pcount != 0)
    {
        // Skipping over the first parenthesis
        ++p;
        if (*(expr + p) == '(')
            ++pcount;
        else if (*(expr + p) == ')')
            --pcount;
    }
    // Case where the open parenthesis has no closing one, return -1 to the calling function
    if (pcount != 0)
        return -1;
    else
        return p;
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
// Function to find the deepest parenthesis (last open parenthesis before first close one), returns -1 if none is found
int find_deepest_parenthesis(char *expr)
{
    int i, open = -1;
    for (i = 0; expr[i] != '\0'; ++i)
    {
        if (expr[i] == '(')
            open = i;
        else if (expr[i] == ')')
            break;
    }
    return open;
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
    double complex variable_values[] = {M_PI, M_E, ans};
    complex double value;
    bool is_negative, is_complex = false;
    int flag = 0;
    char *variable_names[] = {"pi",
                              "exp",
                              "ans"};
    if (expr[start] == '-')
    {
        is_negative = true;
        ++start;
    }
    else
        is_negative = false;

    int i;
    for (i = 0; i < sizeof(variable_names) / sizeof(*variable_names); ++i)
    {
        if (strncmp(expr + start, variable_names[i], strlen(variable_names[i])) == 0)
        {
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
    default:
        return false;
    }
}
// Function that returns the index of the previous operator starting from i
// if no operators are found, returns 0.
int previousop(char *expr, int i)
{
    while (i >= 0)
    {
        if (is_op(expr[i]) == false)
            --i;
        else
            break;
    }
    return i;
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
int nextop(char *expr, int i)
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
bool combine_add_subtract(char *expr, int a, int b)
{
    int subcount, i, j;
    if (b > strlen(expr) || a > b || a < 0)
        return false;
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
            string_resizer(expr, j, i);
        i = find_add_subtract(expr, i + 1);
    }
    return true;
}
// Deletes whitespace from the expression
void remove_whitespace(char *expr)
{
    int start, end, length;
    length = strlen(expr);
    for (start = 0; start < length; ++start)
    {
        if (expr[start] == ' ')
        {
            end = start;
            while (expr[end] == ' ')
                ++end;
            memmove(expr + start, expr + end, length - end + 1);
            length -= end - start;
        }
    }
}
/*
    Utility function to resize a region inside a string
    This function receives the address of the string (*str),
    the index of last element of the region (end), the new index of the last element after resize.
*/
void string_resizer(char *str, int o_end, int n_end)
{
    // case when size remains the same
    if (o_end == n_end)
        return;
    // Other cases
    else
        memmove(str + n_end + 1, str + o_end + 1, strlen(str + o_end));
}
// Simple function that checks if the character is a number
bool is_number(char c)
{
    if (c >= '0' && c <= '9')
        return true;
    else
        return false;
}
bool is_alphabetic(char c)
{
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
        return true;
    else
        return false;
}
/*
 Function that replaces the data present between indexes a and b with the real value v sent by the calling function.
 Returns index of the last element of the number printed.
 */
int value_printer(char *expr, int a, int b, double v)
{
    char temp[24];
    int length;
    // Check if the value was negative then became positive to add a +.
    if (expr[a] == '-' && v >= 0 && a != 0)
        sprintf(temp, "+%.16g", v);
    else
    {
        // case with double negatives ex: --x
        if (a != 0 && expr[a - 1] == '-' && v < 0)
        {
            v = -v;
            expr[a - 1] = '+';
        }
        sprintf(temp, "%.16g", v);
    }
    length = strlen(temp);
    string_resizer(expr, b, a + length - 1);
    strncpy(expr + a, temp, length);
    return a + length - 1;
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
        string_resizer(expr, *p3, *p3 + 3);
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
        string_resizer(expr, *p3, *p3 + 3);
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
    if (is_number(expr[start]) || expr[start] == 'i')
        return true;
    else
        return false;
}
// Function that converts implicit multiplication to explicit multiplication, may change the length of expr
bool implicit_multiplication(char **expr)
{
    int i, j, k, symbol;
    bool condition_met;
    char *expr_ptr;
    *expr = realloc(*expr, 4 * strlen(*expr) * sizeof(char));
    expr_ptr = *expr;
    if (var_implicit_multiplication(expr_ptr) == false)
    {
        *expr = realloc(*expr, (strlen(*expr) + 1) * sizeof(char));
        return false;
    }
    i = next_open_parenthesis(expr_ptr, 0);
    // Implicit multiplication with parenthesis
    while (i != -1)
    {
        symbol = -1;
        if (symbol != -1)
        {
            k = find_closing_parenthesis(expr_ptr, i);
            expr_ptr[i] = '[';
            expr_ptr[k] = ']';
            i = k + 1;
            k = find_closing_parenthesis(expr_ptr, i);
            if (k == -1)
                return false;
            expr_ptr[i] = '[';
            expr_ptr[k] = ']';
            string_resizer(expr_ptr, k, k + 2);
            memmove(expr + symbol + 1, expr + symbol, k - symbol + 1);
            expr_ptr[symbol] = '(';
            expr_ptr[k + 2] = ')';
            i = symbol;
        }
        j = find_closing_parenthesis(expr_ptr, i);
        condition_met = true;

        if (i != 0)
        {
            // Case where the implicit multiplication is with a number
            if (is_number(expr_ptr[i - 1]) || expr_ptr[i - 1] == 'i')
                k = find_startofnumber(expr_ptr, i);
            // Case where the implicit multiplication is with a closed parenthesis
            else if (expr_ptr[i - 1] == ')')
                k = find_opening_parenthesis(expr_ptr, i - 1);
            // Other cases
            else
                condition_met = false;
            if (condition_met)
                shift_and_multiply(expr_ptr, &k, &i, &j, 'l');
        }
        condition_met = true;
        if (expr_ptr[j + 1] != '\0')
        {
            // Case where implicit multiplication is with a number or a variable
            if (is_number(expr_ptr[j + 1]) || is_alphabetic(expr_ptr[j + 1]))
                k = find_endofnumber(expr_ptr, j + 1);
            else if (expr_ptr[j + 1] == '(')
                k = find_closing_parenthesis(expr_ptr, j + 1);
            else
                condition_met = false;
            if (condition_met)
                shift_and_multiply(expr_ptr, &i, &j, &k, 'r');
        }
        i = next_open_parenthesis(expr_ptr, i + 1);
    }
    // Restore parenthesis
    for (i = 0; expr_ptr[i] != '\0'; ++i)
    {
        if (expr_ptr[i] == '[')
            expr_ptr[i] = '(';
        else if (expr_ptr[i] == ']')
            expr_ptr[i] = ')';
    }
    *expr = realloc(*expr, (strlen(*expr) + 1) * sizeof(char));
    return true;
}
bool var_implicit_multiplication(char *expr)
{
    char *keyword[] = {"ans", "expr", "pi", "x", "i", "sin", "ceil", "dx"};
    char *function_name[] =
        {"fact", "abs", "arg", "ceil", "floor", "acosh", "asinh", "atanh", "acos", "asin", "atan", "cosh", "sinh", "tanh", "cos", "sin", "tan", "ln", "log"};
    int keylen[] = {3, 3, 2, 1, 1, 3, 4, 2}, i, current, k, p1, p2, p3, temp;
    // keyword[current] is the current variable being tested
    for (current = 0; current < 5; ++current)
    {
        // search the first match of keyword[current]
        i = s_search(expr, keyword[current], 0);
        while (i != -1)
        {
            // Checking that keyword[current] is not part of another keyword, like x being part of expr
            for (k = 0; k < 8; ++k)
            {
                if (part_of_keyword(expr, keyword[current], keyword[k], i) == true)
                    goto endl1;
            }
            // Implicit multiplication on the left
            if (i != 0)
            {
                // current<4 condition guarantees that no implicit multiplication checks are done on "i" with numbers
                // read_complex can read the complex number without * mark
                if (current < 4)
                {
                    if (is_number(expr[i - 1]))
                    {
                        p1 = find_startofnumber(expr, i - 1);
                        p2 = i;
                        p3 = i + keylen[current] - 1;
                        shift_and_multiply(expr, &p1, &p2, &p3, 'l');
                        i += 2;
                    }
                    else if (expr[i - 1] == ')')
                    {
                        p1 = find_opening_parenthesis(expr, i - 1);
                        p2 = i;
                        p3 = i + keylen[current] - 1;
                        shift_and_multiply(expr, &p1, &p2, &p3, 'l');
                        i += 2;
                    }
                }
                // keyword[k] is the variable being checked if it is being implicitly multiplied with keyword[j]
                for (k = 0; k < 5; ++k)
                {
                    if (i - keylen[k] < 0)
                        continue;
                    temp = strncmp(keyword[k], expr + i - keylen[k], keylen[k]);
                    if (temp == 0)
                    {
                        p1 = i - keylen[k];
                        p2 = i;
                        p3 = i + keylen[current] - 1;
                        shift_and_multiply(expr, &p1, &p2, &p3, 'l');
                        i += 2;
                        break;
                    }
                }
            }
            // Implicit multiplication on the right
            // i + keylen[j] is the index of the first element after the keyword found
            if (expr[i + keylen[current]] != '\0')
            {
                if (current < 4)
                {
                    if (is_number(expr[i + keylen[current]]) == true)
                    {
                        p1 = i;
                        p2 = i + keylen[current] - 1;
                        p3 = find_endofnumber(expr, p2 + 1);
                        shift_and_multiply(expr, &p1, &p2, &p3, 'r');
                        ++i;
                    }
                    else if (expr[i + keylen[current]] == '(')
                    {
                        p1 = i;
                        p2 = i + keylen[current] - 1;
                        p3 = find_closing_parenthesis(expr, p2 + 1);
                        shift_and_multiply(expr, &p1, &p2, &p3, 'r');
                        ++i;
                    }
                }
                // keyword[k] is the variable being checked if it is being implicitly multiplied with keyword[j]
                for (k = 0; k < 5; ++k)
                {
                    temp = strncmp(keyword[k], expr + i + keylen[current], keylen[k]);
                    if (temp == 0)
                    {
                        p1 = i;
                        p2 = i + keylen[current] - 1;
                        p3 = p2 + keylen[k];
                        shift_and_multiply(expr, &p1, &p2, &p3, 'r');
                        break;
                    }
                }
            }
        endl1:;
            // Find next match of keyword[j]
            i = s_search(expr, keyword[current], i + keylen[current]);
        }
    }
    int length;

    // Implicit multiplication with scientific functions
    for (current = 0; current < 19; ++current)
    {
        length = strlen(function_name[current]);
        i = s_search(expr, function_name[current], 0);
        while (i != -1)
        {
            p2 = i;
            if (i != 0)
            {
                // Checking that the keyword is not a part of another keyword
                for (k = 4; k < 16; ++k)
                {
                    if (part_of_keyword(expr, function_name[current], function_name[k], i) == true)
                        goto endl2;
                }
            }
            // Add enclosing parenthesis for cases like cos x
            if (expr[i + length] != '(')
            {
                int start = i + length, end;
                if (expr[start] == '\0' || (expr[start] != '-' && expr[start] != '+' && is_op(expr[start])))
                {
                    error_handler(PARAMETER_MISSING, 1, 1, start);
                    return false;
                }
                // Number following a scientific function
                if (is_valid_number(expr, start))
                    end = find_endofnumber(expr, start);
                else if (is_op(expr[start + 1]) == false)
                {
                    // Variable following a scientific function
                    end = nextop(expr, start + 1) - 1;
                    if (end == -2)
                        end = strlen(expr) - 1;
                    if (end == start - 1)
                    {
                        error_handler(PARAMETER_MISSING, 1, 1, start);
                        return false;
                    }
                }
                // Shifting after the function to make room for enclosing parenthesis
                string_resizer(expr, end, end + 2);
                memmove(expr + start + 1, expr + start, end - start + 1);
                expr[start] = '(';
                expr[end + 2] = ')';
                p3 = end + 2;
            }
            // Implicit multiplication on the left
            if (i != 0)
            {
                if (is_number(expr[i - 1]) == false && is_op(expr[i - 1]) == false)
                {
                    bool success = false;
                    for (k = 0; k < 4; ++k)
                    {
                        // Verifying that the scientific function is not preceded by any variable (ans,pi,expr,x)
                        if (i >= keylen[k] && r_search(expr, keyword[k], i, true) != i - keylen[k])
                        {
                            p1 = i - keylen[k];
                            success = true;
                            break;
                        }
                    }
                    // Case where a scientific function is preceded by another scientific function (or possibly garbage)
                    if (success == false)
                    {
                        string_resizer(expr, p3, p3 + 2);
                        memmove(expr + p2 + 1, expr + p2, p3 - p2 + 1);
                        // Encasing the function in parenthesis
                        expr[p2] = '(';
                        expr[p3 + 2] = ')';
                        i = s_search(expr, function_name[current], i + length + 1);
                        continue;
                    }
                }
                // Case where the scientific function is preceded by a number
                // Add a * sign after the number
                else if (is_number(expr[i - 1]) || expr[i - 1] == 'i')
                {
                    string_resizer(expr, i - 1, i);
                    expr[i++] = '*';
                }
            }
        // Initializing the next run
        endl2:;
            i = s_search(expr, function_name[current], i + length);
        }
    }
    return true;
}
// Function to find the index of the number's end
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
        if (is_number(expr[end + 1]) == true && expr[end + 1] != '\0')
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
    * Starting from end:
    * If start=0, break.
    * Check if the char at start-1 is a number, if true decrement start.
    * If not handle the following cases:
    * expr[start-1] is the imaginary number 'i': decrement start and break (this means i is the start of the number)
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
        if (is_number(expr[start - 1]) == true)
            --start;
        else
        {
            if (expr[start - 1] == 'i')
            {
                --start;
                break;
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
// Function to find the first occurence of a string starting from i
int s_search(char *source, char *keyword, int index)
{
    int length = strlen(keyword);
    while (source[index] != '\0')
    {
        if (source[index] == keyword[0])
        {
            if (strncmp(source + index, keyword, length) == 0)
                return index;
            else
                ++index;
        }
        else
            ++index;
    }
    return -1;
}
// Reverse search function, returns the index of the first match of keyword, or -1 if none is found
// adjacent_search stops the search if the index is not inside the keyword
int r_search(char *source, char *keyword, int index, bool adjacent_search)
{
    int length = strlen(keyword), i = index;
    while (i != -1)
    {
        if (adjacent_search == true && i == index - length)
            return -1;
        if (source[i] == keyword[0])
        {
            if (strncmp(source + i, keyword, length) == 0)
                return i;
            else
                --i;
        }
        else
            --i;
    }
    return -1;
}

// Function to help with printing formatted equations to stdout.
void nice_print(char *format, double value, bool is_first)
{
    // Printing nothing if the value is 0 or -0
    if (fabs(value) == 0)
        return;
    if (is_first == true)
    {
        if (value > 0)
            printf(" ");
        else
            printf(" - ");
    }
    else
    {
        if (value > 0)
            printf(" + ");
        else
            printf(" - ");
    }
    printf(format, fabs(value));
}
// Find the second deepest parenthesis
int second_deepest_parenthesis(char *expr)
{
    int i, open, closed;
    /*
    To find the second deepest parenthesis:
    * Find the deepest parenthesis
    * Search the open parenthesis preceding it.
    * If there is no close parenthesis between these 2 parenthesis, return
    */
    i = find_deepest_parenthesis(expr);
    // Case where the deepest parenthesis is the first one,
    if (i == 0)
        return -1;
    while (i > 0)
    {
        open = previous_open_parenthesis(expr, i - 1);
        closed = previous_closed_parenthesis(expr, i);
        // The only case where closed==open is when no open or closed parenthesis is preceding the position
        if (closed == open)
            return -1;
        if (closed > open)
        {
            i = open;
            continue;
        }
        else
            return open;
    }
    return -1;
}
int previous_open_parenthesis(char *expr, int p)
{
    while (expr[p] != '(' && p > 0)
        --p;
    // Checking if the stop was caused by reaching 0 or finding a parenthesis
    if (p == 0 && expr[p] != '(')
        return -1;
    else
        return p;
}
int previous_closed_parenthesis(char *expr, int p)
{
    while (expr[p] != ')' && p > 0)
        --p;
    // Checking if the stop was caused by reaching 0 or finding a parenthesis
    if (p == 0 && expr[p] != ')')
        return -1;
    else
        return p;
}
int next_open_parenthesis(char *expr, int p)
{
    while (expr[p] != '(' && expr[p] != '\0')
        ++p;
    // Checking if the stop was caused by reaching \0 or finding a parenthesis
    if (expr[p] == '\0')
        return -1;
    else
        return p;
}
int next_closed_parenthesis(char *expr, int p)
{
    while (expr[p] != ')' && expr[p] != '\0')
        ++p;
    // Checking if the stop was caused by reaching \0 or finding a parenthesis
    if (expr[p] == '\0')
        return -1;
    else
        return p;
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
// Function to sort ints in increasing order for qsort
int compare_ints(const void *a, const void *b)
{
    if (*(int *)a < *(int *)b)
        return -1;
    else if (*(int *)a > *(int *)b)
        return 1;
    else
        return 0;
}
// Reverse of the above
int compare_ints_reverse(const void *a, const void *b)
{
    if (*(int *)a < *(int *)b)
        return 1;
    else if (*(int *)a > *(int *)b)
        return -1;
    else
        return 0;
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
    open = next_open_parenthesis(expr, 0);
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
        open = next_open_parenthesis(expr, open + 1);
        ++k;
    }
    qsort(open_position, k, sizeof(int), compare_ints);
    qsort(close_position, k, sizeof(int), compare_ints_reverse);
    // Case of no open parenthesis, check if a close parenthesis is present
    if (k == 0)
        close = next_closed_parenthesis(expr, 0);
    else
        close = next_closed_parenthesis(expr, close_position[0] + 1);

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

/*
Function that compares the priority of 2 operators, returns:
1 if operator1 has higher priority than operator2
0 if operators 1 and 2 have same priority
-1 if operator1 has lower priority than operator2
*/
int priority_test(char operator1, char operator2)
{
    char operators[7] = {'!', '^', '*', '/', '%', '+', '-'};
    short priority[7] = {4, 3, 2, 2, 2, 1, 1}, op1_p = '\0', op2_p = '\0', i;
    // Find priority of LeftOperand
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
    if (op1_p > op2_p)
        return 1;
    if (op1_p == op2_p)
        return 0;
    if (op1_p < op2_p)
        return -1;
    // Silencing the compiler warning
    exit(-1);
}
// Replaces the keyword with the appropriate value
void var_to_val(char *expr, char *keyword, double value)
{
    int i = 0, keylen = strlen(keyword);
    while (expr[i] != '\0')
    {
        // search for the next occurence of the keyword
        i = s_search(expr, keyword, i);
        if (i == -1)
            return;
        // print the value in place of the keyword
        i = value_printer(expr, i, i + keylen - 1, value);
    }
}
// Function that extracts arguments separated by "," from a string and returns them in a struct
arg_list *get_arguments(char *string)
{
    arg_list *current_args = malloc(sizeof(arg_list));
    int length = strlen(string), current, prev, max_args = 10, count;
    // You could use current_args.arg_count but this improves readability (the compiler should optimize this)
    count = 0;
    current_args->arguments = malloc(max_args * sizeof(char *));
    for (current = prev = 0; current < length; ++current)
    {
        if (current == max_args)
        {
            max_args += 10;
            current_args = realloc(current_args, max_args * sizeof(arg_list));
        }
        if (string[current] == ',')
        {
            current_args->arguments[count] = malloc(current - prev + 1);
            strncpy(current_args->arguments[count], string + prev, current - prev);
            current_args->arguments[count][current - prev] = '\0';
            prev = current + 1;
            ++count;
        }
    }
    current_args->arguments[count] = malloc(current - prev + 1);
    strncpy(current_args->arguments[count], string + prev, current - prev);
    current_args->arguments[count][current - prev] = '\0';
    ++count;
    current_args->arg_count = count;
    return current_args;
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