/*
Copyright (C) 2023-2024 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/

#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "libtmsolve.h"

void test_scientific(char *buffer)
{
    int field_separator = tms_f_search(buffer, ";", 0, false);
    if (field_separator == -1)
    {
        fputs("Incorrect format for test file, missing ;", stderr);
        return;
    }
    char expr[field_separator + 1];
    double complex expected_ans;
    // Not the most efficient way, but the calculator should be reliable for simple ops.
    expected_ans = tms_solve(buffer + field_separator + 1);

    strncpy(expr, buffer, field_separator);
    expr[field_separator] = '\0';
    puts(expr);

    tms_g_ans = tms_solve(expr);
    if (isnan(creal(tms_g_ans)))
    {
        tms_error_handler(EH_PRINT);
        exit(1);
    }

    // Relative error in case the expected answer is 0
    if (expected_ans == 0)
    {
        if (fabs(creal(expected_ans)) < 1e-15 && fabs(cimag(expected_ans)) < 1e-15)
            puts("Passed\n--------------------\n");
        else
            fputs("Expected answer is zero but actual answer is non negligible.", stderr);
    }
    else
    {
        double relative_error = cabs((expected_ans - tms_g_ans) / expected_ans);
        printf("relative error=%g\n", relative_error);
        if (relative_error > 1e-3)
        {
            fputs("Relative error is too high.", stderr);
            exit(1);
        }
        else
            puts("Passed\n--------------------\n");
    }
}

void test_base_n(char *buffer)
{
    int field_separator = tms_f_search(buffer, ";", 0, false);
    if (field_separator == -1)
    {
        fputs("Incorrect format for test file, missing ;", stderr);
        return;
    }
    char expr[field_separator + 1];
    int64_t expected_ans;

    // Read the answer to expect
    if (tms_int_solve(buffer + field_separator + 1, &expected_ans) == -1)
    {
        tms_error_handler(EH_PRINT);
        exit(1);
    }
    expected_ans = tms_sign_extend(expected_ans);

    strncpy(expr, buffer, field_separator);
    expr[field_separator] = '\0';
    puts(expr);
    // Solve the expression
    if (tms_int_solve(expr, &tms_g_int_ans) == -1)
    {
        tms_error_handler(EH_PRINT);
        exit(1);
    }
    tms_g_int_ans = tms_sign_extend(tms_g_int_ans);

    if (tms_g_int_ans == expected_ans)
        puts("Passed\n--------------------\n");
    else
    {
        puts("Unexpected result");
        exit(1);
    }
}

int main(int argc, char **argv)
{
    _tms_debug = 1;
    if (argc < 2)
    {
        puts("Missing argument\nUsage: tms_test test_file");
        exit(1);
    }
    // Load the test file, should have the following format:
    // Mode_char:expression1;expected_answer1
    // Mode_char is either S or B

    FILE *test_file;
    char mode;
    test_file = fopen(argv[1], "r");
    if (test_file == NULL)
    {
        fputs("Unable to open test file.\n", stderr);
        exit(1);
    }

    // I won't make lines longer than that to test
    char buffer[1000];

    while (feof(test_file) == 0)
    {
        fgets(buffer, 1000, test_file);

        tms_remove_whitespace(buffer);
        mode = buffer[0];
        switch (mode)
        {
        case 'S':
            test_scientific(buffer + 2);
            break;

        case 'B':
            test_base_n(buffer + 2);
            break;

        default:
            fputs("Invalid test mode.\n", stderr);
        }
    }
}
