/*
Copyright (C) 2021-2022 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
/*
#include "internals.h"
#include "scientific.h"
#include "string_tools.h"
#include "matrix.h"

//Solve a linear system Ax=b
void system_solver(double *A, double *b, int degree)
{
    double *system, temp, *copy, det;
    int i, row, column;
    det = matrix_det(system);
    if (det == 0)
    {
        puts("The system has no solution or infinite solutions.");
        return;
    }
    //Calculating solutions
    for (i = 0; i < degree * (degree + 1); ++i)
    {
        row = i / (degree + 1);
        column = i % (degree + 1);
        if (column == degree)
            printf(" = %.14g\n", B[row]);
        else
        {
            if (*(system + row * degree + column + 2) >= 0)
            {
                if (column != 0)
                    printf(" + ");
                printf("%.14g x%d", *(system + row * degree + column + 2), column + 1);
            }
            else if (*(system + row * degree + column + 2) < 0)
                printf(" - %.14g x%d", -*(system + row * degree + column + 2), column + 1);
        }
    }
    //Calculating and showing the solution
    puts("\nSolution:\n");
    for (i = 0; i < degree; ++i)
    {
        copy = matrix_copy(system);
        replace_matrix_column(copy, B, i);
        temp = matrix_det(copy) / det;
        printf("x%d = %.14g\n", i + 1, temp);
        free(copy);
    }
    printf("\n");
    free(system);
}

void equation_solver(int degree)
{
    double a, b, c, d, p, q, delta;
    double complex x1, x2, x3, cdelta;
    switch (degree)
    {
    case 1:
        puts("Degree 1 equation a x = b\nEnter a:");
        do
        {
            a = get_value(NULL);
            if (a == 0)
                puts("a must not be zero.");
        } while (a == 0);
        puts("Enter b:");
        b = get_value(NULL);
        printf("Equation: %.10g x = %.10g\n", a, b);
        printf("Solution: x = %.10g\n", b / a);
        break;
    case 2:
        puts("Degree 2 equation a x^2 + b x + c = 0\nEnter a:");
        do
        {
            a = get_value(NULL);
            if (a == 0)
                puts("a must not be zero.");
        } while (a == 0);
        puts("Enter b:");
        b = get_value(NULL);
        puts("Enter c:");
        c = get_value(NULL);
        printf("\nEquation:");
        nice_print("%.10g x^2", a, true);
        nice_print("%.10g x", b, false);
        nice_print("%.10g", c, false);
        printf(" = 0\nSolutions:\n");
        delta = b * b - 4 * a * c;
        if (delta == 0)
        {
            printf("x1 = x2 = %.10g", -b / (2 * a));
        }
        else if (delta > 0)
        {
            printf("x1 = %.10g\n", (-b - sqrt(delta)) / (2 * a));
            printf("x2 = %.10g", (-b + sqrt(delta)) / (2 * a));
        }
        else if (delta < 0)
        {
            printf("x1 =");
            x1 = -b / (2 * a) + I * sqrt(-delta) / (2 * a);
            complex_print(x1);
            printf("\nx2 =");
            x2 = conj(x1);
            complex_print(x2);
        }
        printf("\n");
        break;
    case 3:
        puts("Degree 3 equation a x^3 + b x^2 +c x + d = 0\nEnter a:");
        do
        {
            a = get_value(NULL);
            if (a == 0)
                puts("a must not be zero.");
        } while (a == 0);
        puts("Enter b:");
        b = get_value(NULL);
        puts("Enter c:");
        c = get_value(NULL);
        puts("Enter d:");
        d = get_value(NULL);
        p = (3 * a * c - b * b) / (3 * a * a);
        q = (2 * b * b * b - 9 * a * b * c + 27 * a * a * d) / (27 * a * a * a);
        //delta1
        cdelta = q * q + (4 * p * p * p) / 27;
        cdelta = csqrt(cdelta);
        x1 = cbrt((-q - cdelta) / 2) + cbrt((-q + cdelta) / 2) - b / (3 * a);
        //delta2
        cdelta = cpow(b + a * x1, 2) - 4 * a * (c + x1 * (b + a * x1));
        cdelta = csqrt(cdelta);
        x2 = (-b - a * x1 - cdelta) / (2 * a);
        x3 = (-b - a * x1 + cdelta) / (2 * a);
        printf("\nEquation: ");
        nice_print("%.10g x^3", a, true);
        nice_print("%.10g x^2", b, false);
        nice_print("%.10g x", c, false);
        nice_print("%.10g", d, false);
        printf(" = 0\nSolutions:\n");
        if (x1 == x2 && x1 == x3)
            printf("x1 = x2 = x3 = %.10g", creal(x1));
        else if (x2 == x3)
        {
            printf("x1 =");
            complex_print(x1);
            printf("\nx2 = x3 =");
            complex_print(x2);
        }
        else
        {
            printf("x1 =");
            complex_print(x1);
            printf("\nx2 =");
            complex_print(x2);
            printf("\nx3 =");
            complex_print(x3);
        }
        printf("\n");
        break;
    default:
        puts("Degree not supported.");
    }
}
*/