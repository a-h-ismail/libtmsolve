/*
Copyright (C) 2021-2022 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "internals.h"
#include "string_tools.h"
#include "scientific.h"
void complex_interpreter(char *exp)
{
    int p1, p2;
    double module, argument;
    bool success;
    if (exp[0] == '\0')
    {
        error_handler("Empty input.", 1, 1, -1);
        return;
    }
    if (parenthesis_check(exp) == false)
    {
        printf("\n");
        return;
    }
    if (implicit_multiplication(exp) == false)
        return;
    if ((p1 = s_search(exp, "inf", 0)) != -1)
    {
        error_handler("Infinity is not accepted as input (inf).", 1, 1, p1);
        return;
    }
    variable_matcher(exp);
    /*
    *Given that complex numbers are made of 2 parts, real and imaginary, the interpreter has to:
    *Find the second deepest open parenthesis and its closing parenthesis.
    *Solve the content of each () inside.
    *Develop the content of the second deepest parenthesis pair to get rid of the deepest parenthesis inside.
    *Reduce the content of the () already mentioned.
    */
    combine_add_subtract(exp, 0, strlen(exp) - 1);
    while (1)
    {
        p1 = second_deepest_parenthesis(exp);
        //Case where there is no second deepest parenthesis
        if (p1 == -1)
        {
            success = complex_stage1_solver(exp, 0, strlen(exp) - 1);
            if (success == false)
                return;
            else
                break;
        }
        else
            p2 = find_closing_parenthesis(exp, p1);
        success = complex_stage1_solver(exp, p1 + 1, p2 - 1);
        if (success == false)
            return;
    }
    if (valid_result(exp) == false)
        return;
    ans = read_complex(exp, 0, strlen(exp) - 1);
    printf("= ");
    complex_print(ans);
    module = cabs(ans);
    argument = carg(ans);
    printf("\nModulus: %.10g\nArgument: %.10g rad = %.10g pi rad = %.10g deg\n", module, argument, argument / M_PI, argument * (180 / M_PI));
}
//These 2 functions are to fix incompatible pointer type in *c_functions[] initialization below
double complex c_cabs(double complex z)
{
    return cabs(z);
}
double complex c_carg(double complex z)
{
    return carg(z);
}
bool complex_stage1_solver(char *exp, int a, int b)
{
    int i, j, k, keylen;
    char *temp, *function_name[] =
                    {"abs", "arg", "acosh", "asinh", "atanh", "acos", "asin", "atan",
                     "cosh", "sinh", "tanh", "cos", "sin", "tan", "log"};
    double complex v, (*c_functions[])(double complex) =
                          {c_cabs, c_carg, cacosh, casinh, catanh, cacos, casin, catan, ccosh, csinh, ctanh, ccos, csin, ctan, clog};
    temp = (char *)malloc(23 * sizeof(char));
    strncpy(temp, exp + a, b - a + 1);
    temp[b - a + 1] = '\0';
    //Solve scientific functions in cases where the value is not enclosed in parenthesis
    for (k = 0; k < 15; ++k)
    {
        i = s_search(temp, function_name[k], 0);
        keylen = strlen(function_name[k]);
        while (i != -1)
        {
            if (temp[i + keylen] == '(')
            {
                i = s_search(temp, function_name[k], i + keylen);
                continue;
            }
            j = find_endofnumber(temp, i + keylen);
            v = read_complex(temp, i + keylen, j);
            if (isnan(creal(v)) == true || isnan(cimag(v)) == true)
                return false;
            //vtemp is used to be able to collect the result of function
            v = (*c_functions[k])(v);
            if (creal(v) != 0 && cimag(v) != 0)
            {
                temp[i] = '(';
                temp[j] = ')';
                i = s_complex_print(temp, i + 1, j - 1, v);
            }
            else
                i = s_complex_print(temp, i, j, v);
            i = s_search(temp, function_name[k], i + 1);
        }
    }
    //Use stage 2 solver to solve inside every ()
    i = next_open_parenthesis(temp, 0);
    while (i != -1)
    {
        j = find_closing_parenthesis(temp, i);
        i = complex_stage2_solver(temp, i + 1, j - 1);
        if (i == -1)
            return false;
        i = next_open_parenthesis(temp, i);
    }
    //Solve scientific functions that are in parenthesis
    for (k = 0; k < 15; ++k)
    {
        i = s_search(temp, function_name[k], 0);
        keylen = strlen(function_name[k]);
        while (i != -1)
        {
            j = find_closing_parenthesis(temp, i + keylen + 1);
            v = read_complex(temp, i + keylen + 1, j - 1);
            if (isnan(creal(v)) == true || isnan(cimag(v)) == true)
                return false;
            //vtemp is used to be able to collect the result of function
            v = (*c_functions[k])(v);
            if (creal(v) != 0 && cimag(v) != 0)
            {
                temp[i] = '(';
                temp[j] = ')';
                i = s_complex_print(temp, i + 1, j - 1, v);
            }
            else
                i = s_complex_print(temp, i, j, v);
            i = s_search(temp, function_name[k], i + 1);
        }
    }
    //Solve the composed expressions made of parenthesis like ()/() , ()/ ...
    for (i = 0; temp[i] != 0; ++i)
    {
        if (temp[i] == '^')
        {
            i = c_process(temp, i);
            if (i == -1)
                return false;
        }
    }
    for (i = 0; temp[i] != '\0'; ++i)
    {
        if (temp[i] == '*' || temp[i] == '/')
        {
            i = c_process(temp, i);
            if (i == -1)
                return false;
        }
    }
    //Remove inner parenthesis to reach a developed expression
    //Search for the first open parenthesis
    i = next_open_parenthesis(temp, 0);
    while (i != -1)
    {
        //No parenthesis found, break.
        if (i == -1)
            break;
        else
            j = find_closing_parenthesis(temp, i);
        if (i == 0 || temp[i - 1] == '+')
        {
            temp[i] = ' ';
            temp[j] = ' ';
            i = j + 1;
        }
        //If the parenthesis is preceded by a - , flip signs
        else if (temp[i - 1] == '-')
        {
            temp[i] = ' ';
            temp[j] = ' ';
            k = find_add_subtract(temp, i);
            while (k < j)
            {
                if (k == -1)
                    break;
                if (temp[k] == '+')
                    temp[k] = '-';
                else
                    temp[k] = '+';
                k = find_add_subtract(temp, k + 1);
            }
            temp[i - 1] = ' ';
        }
        else
        {
            error_handler("Syntax error.", 1, 0);
            return false;
        }
        combine_add_subtract(temp, 0, strlen(temp) - 1);
        //Search for the next open parenthesis
        i = next_open_parenthesis(temp, i);
    }
    string_cleaner(temp);
    //Use stage 2 solver to solve the developed expression
    k = complex_stage2_solver(temp, 0, strlen(temp) - 1);
    if (k == -1)
        return false;
    //Copy the final result to exp
    string_resizer(exp, b, a + strlen(temp) - 1);
    strncpy(exp + a, temp, strlen(temp));
    free(temp);
    return true;
}
//Stage 2 solver has the role of solving and reducing a complex expression that has no parenthesis
int complex_stage2_solver(char *exp, int a, int b)
{
    int i, j, length;
    char *temp = (char *)malloc(23*(b - a + 1) * sizeof(char));
    double complex result = 0;
    strncpy(temp, exp + a, b - a + 1);
    temp[b - a + 1] = '\0';
    //Step 1: Solve elemental *,/,^
    for (i = 0; temp[i] != 0; ++i)
    {
        if (temp[i] == '^')
        {
            i = c_process(temp, i);
            if (i == -1)
                return -1;
        }
    }
    for (i = 0; temp[i] != 0; ++i)
    {
        if (temp[i] == '*' || temp[i] == '/')
        {
            i = c_process(temp, i);
            if (i == -1)
                return -1;
        }
    }
    //Step 2: Go through the string and reduce the expression
    i = 0;
    length = strlen(temp);
    while (i < length)
    {
        j = find_add_subtract(temp, i + 1) - 1;
        //case where no more + or - is found, take last term which ends at length-1
        if (j == -2)
            j = length - 1;
        result += read_complex(temp, i, j);
        //Error checking
        if (isnan(creal(result)) == true)
            return -1;
        //Preparing the next iteration
        i = j + 1;
    }
    b = s_complex_print(exp, a, b, result);
    free(temp);
    //Send the new ending index to the calling function to prevent its counter from shooting over unsolved parts
    return b;
}
//Prints a complex number in the region limited by a and b, returns index of the last element of the number printed
int s_complex_print(char *exp, int a, int b, double complex value)
{
    char buffer[50];
    int length;
    double real = creal(value), imaginary = cimag(value);
    //Case with a pure real value, use value_printer
    if (imaginary == 0)
        return value_printer(exp, a, b, real);
    else
    {
        //Printing the real part first
        //Flipping the sign '-' to '+' if the value is positive
        if (a != 0 && exp[a] == '-' && real > 0)
            sprintf(buffer, "%+.16g", real);
        else
            sprintf(buffer, "%.16g", real);
        //Printing the imaginary part
        //Case with pure imaginary number
        if (real == 0)
        {
            if (a != 0 && exp[a] == '-' && imaginary > 0)
                sprintf(buffer, "%+.14gi", imaginary);
            else
                sprintf(buffer, "%.16gi", imaginary);
        }
        //Case with real and imaginary values
        else
        {
            length = strlen(buffer);
            if (imaginary >= 0)
                sprintf(buffer + length, "+%.16gi", imaginary);
            else
                sprintf(buffer + length, "%.16gi", imaginary);
        }
        length = strlen(buffer);
        string_resizer(exp, b, a + length - 1);
        strncpy(exp + a, buffer, length);
        if (a != 0)
            combine_add_subtract(exp, a - 1, a + length - 1);
        else
            combine_add_subtract(exp, a, a + length - 1);
    }
    return a + length - 1;
}
//Simple function to print a complex number in a clean way to stdout
void complex_print(double complex value)
{
    double real = creal(value), imaginary = cimag(value);
    nice_print("%.10g", real, true);
    if (real == 0)
    {
        if (imaginary == 1)
            printf("i");
        else if (imaginary == -1)
            printf("- i");
        else
            nice_print("%.10g i", cimag(value), true);
    }
    else
    {
        if (imaginary == 1)
            printf(" + i");
        else if (imaginary == -1)
            printf(" - i");
        else
            nice_print("%.10g i", cimag(value), false);
    }
}
//Function to calculate complex multiply, divide, power. On success, returns index of the last element of the result, on failure, returns -1
int c_process(char *exp, int p)
{
    int a, b;
    char op;
    double complex v1, v2, result = 0;
    op = exp[p];
    if (exp[p - 1] != ')' && exp[p + 1] != '(')
    {
        //Calculate the elemental multiply, divide and power
        if (op == '*' || op == '/' || op == '^')
        {
            a = find_startofnumber(exp, p - 1);
            b = find_endofnumber(exp, p + 1);
            v1 = read_complex(exp, a, p - 1);
            if (isnan(creal(v1)) == true)
                return -1;
            v2 = read_complex(exp, p + 1, b);
            if (isnan(creal(v2)) == true)
                return -1;
            switch (op)
            {
            case '*':
                result = v1 * v2;
                break;
            case '/':
                result = v1 / v2;
                break;
            case '^':
                //case of 1 term without parenthesis like -i^n
                //equivalent to -(i^n)
                if (creal(v1) < 0 || cimag(v1) < 0)
                {
                    v1 = -v1;
                    ++a;
                }
                result = cpow(v1, v2);
                //Getting rid of slight lack of precision of cpow() funtion
                //I don't think mathematicians will appreciate seeing i^2 = - 1 + 1.224646799e-16 i or exp^(i*pi/2) = 2.293182665e-14 + i
                if (fabs(creal(result) / cimag(result)) < 1e-12)
                    result = cimag(result);
                if (fabs(cimag(result) / creal(result)) < 1e-12)
                    result = creal(result);
                break;
            }
            return s_complex_print(exp, a, b, result);
        }
    }
    //Case of operations on composed values inside parenthesis like ()/() ./() ()/.
    else
    {
        if (op == '*' || op == '/' || op == '^')
        {
            if (exp[p - 1] == ')')
            {
                a = previous_open_parenthesis(exp, p - 1);
                v1 = read_complex(exp, a + 1, p - 2);
                if (isnan(creal(v1)) == true)
                    return -1;
            }
            else
            {
                a = find_startofnumber(exp, p - 1);
                v1 = read_complex(exp, a, p - 1);
                if (isnan(creal(v1)) == true)
                    return -1;
            }
            if (exp[p + 1] == '(')
            {
                b = find_closing_parenthesis(exp, p + 1);
                v2 = read_complex(exp, p + 2, b - 1);
                if (isnan(creal(v2)) == true)
                    return -1;
            }
            else
            {
                b = find_endofnumber(exp, p + 1);
                v2 = read_complex(exp, p + 1, b);
                if (isnan(creal(v2)) == true)
                    return -1;
            }
            switch (op)
            {
            case '*':
                result = v1 * v2;
                break;
            case '/':
                if (v2 != 0)
                    result = v1 / v2;
                else
                {
                    error_handler("Cannot divide by zero.", 1, 1, -1);
                    return -1;
                }
                break;
            case '^':
                result = cpow(v1, v2);
                //Getting rid of slight lack of precision
                if (fabs(creal(result) / cimag(result)) < 1e-12)
                    result = I * cimag(result);
                if (fabs(cimag(result) / creal(result)) < 1e-12)
                    result = creal(result);
                break;
            }
            exp[a] = '(';
            exp[b] = ')';
            return s_complex_print(exp, a + 1, b - 1, result);
        }
    }
    return -1;
}
//Function to check if the value between a and b is the real or imaginay part of a number
bool is_imaginary(char *exp, int a, int b)
{
    if (exp[a] == 'i' || exp[b] == 'i')
        return true;
    else
        return false;
}
//Reads the complex number in the string situated between a and b
double complex read_complex(char *exp, int a, int b)
{
    double real = 0, imaginary = 0, temp, success;
    int i;
    temp = s_search(exp, "inf", a);
    if (temp >= a && temp <= b)
    {
        error_handler("Error: Number too big for representation.", 1, 1, -1);
        //NaN is just used here to signal an error, the number is actually bigger than double precision floats
        return NAN;
    }
    //Find the first part of the number
    i = find_endofnumber(exp, a);
    if (is_imaginary(exp, a, i) == false)
        success = sscanf(exp + a, "%lf", &real);
    else
    {
        //Case where the complex number i is used alone (i or -i), workaround because sscanf doesn't find a number to read
        if (b - a <= 1 && is_number(exp[a]) == false && is_number(exp[b]) == false)
        {
            success = 1;
            if (exp[a] != '-')
                imaginary = 1;
            else
                imaginary = -1;
        }
        else
        {
            //Checking if the complex has 2 imaginary numbers
            if (a != i && exp[a] == 'i' && exp[i] == 'i')
            {
                error_handler("Error: a complex number has 2 imaginary numbers in it.", 1, 0);
                //While the number with 2 complex values has an actual value, NAN is just used to signal an error
                //Maybe someone accidentally used 2 'i', aborting in this situation is the best option in my opinion
                return NAN;
            }
            //Scanning according to the position of i
            if (exp[a] == 'i')
                success = sscanf(exp + a, "i%lf", &imaginary);
            else
                success = sscanf(exp + a, "%lfi", &imaginary);
        }
    }
    //Error handling
    if (success != 1)
    {
        error_handler("Error while reading complex number, check syntax.", 1, 0);
        return NAN;
    }
    //If i < b, this means that the number has another part
    if (i < b)
    {
        ++i;
        //If the second part of the number is not preceded by + or - then it isn't valid
        if (exp[i] != '+' && exp[i] != '-')
        {
            error_handler("Invalid complex number, check syntax.", 1, 0);
            return NAN;
        }
        //Using temp just in case this function received a number of the form a+b or ia+ib to prevent ovewriting previously found values
        if (is_imaginary(exp, i, b) == false)
        {
            success = sscanf(exp + i, "%lf", &temp);
            real += temp;
        }
        else
        {
            if (b - i <= 1 && is_number(exp[i]) == false && is_number(exp[b]) == false)
            {
                if (exp[i] != '-')
                    temp = 1;
                else
                    temp = -1;
            }
            else
            {
                //Scanning according to the position of i
                if (exp[i] == 'i')
                    success = sscanf(exp + i, "i%lf", &temp);
                else
                    success = sscanf(exp + i, "%lfi", &temp);
            }
            imaginary += temp;
        }
    }
    if (success != 1)
    {
        error_handler("Error while reading complex number, check syntax.", 1, 0);
        return NAN;
    }
    return real + I * imaginary;
}
/*double complex read_complex(char *exp, int start)
{
    int imaginaryCount = 0, end, status, variable_nameLen[] = {2, 3, 3};
    double complex value, variable_values[] = {M_PI, M_E, ans};
    double temp;
    bool isNegative;
    char *variable_names[] = {"pi",
                              "exp",
                              "ans"};
    if (exp[start] == '-')
    {
        isNegative = true;
        ++start;
    }
    else
        isNegative = false;
    end = find_endofnumber(exp, start);
    if (exp[start] == 'i')
    {
        ++imaginaryCount;
        ++start;
    }
    if (exp[end + 1] == 'i')
        ++imaginaryCount;
    status = sscanf(exp + start, "%lf", &temp);
    value = temp;
    if (status == 0)
    {
        int i;
        for (i = 0; i < 3; ++i)
        {
            if (strncmp(exp + start, variable_names[i], variable_nameLen[i]) == 0)
            {
                value = variable_values[i];
                status = 2;
                break;
            }
        }
    }
    //Case where nothing was found even after checking for variables
    if (status == 0)
        return NAN;
    if (isNegative)
        value = -value;
    while(imaginaryCount!=0)
    {
        value *= I;
        --imaginaryCount;
    }
    return value;
}
*/
