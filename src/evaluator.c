/*
Copyright (C) 2023-2024 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "evaluator.h"
#include "internals.h"
#include "parser.h"
#include "int_parser.h"
#include "scientific.h"
#include "string_tools.h"
#include "tms_complex.h"
#include "bitwise.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

double complex tms_evaluate(tms_math_expr *M)
{
    // No NULL pointer dereference allowed.
    if (M == NULL)
        return NAN;

    tms_op_node *i_node;
    int s_index = 0, s_count = M->subexpr_count;
    // Subexpression pointer to access the subexpression array.
    tms_math_subexpr *S = M->subexpr_ptr;
    while (s_index < s_count)
    {
        // Extended functions have no nodes
        if (S[s_index].nodes == NULL)
        {
            if (S[s_index].exec_extf)
            {
                char *arguments;
                bool _debug_state = _tms_debug;
                tms_arg_list *L;

                int length = S[s_index].solve_end - S[s_index].solve_start + 1;

                // Copy arguments
                arguments = tms_strndup(M->local_expr + S[s_index].solve_start, length);
                L = tms_get_args(arguments);

                // Disable debug output for extended functions
                _tms_debug = false;

                // Call the extended function using its pointer
                **(S[s_index].result) = (*(S[s_index].func.extended))(L);

                _tms_debug = _debug_state;

                if (isnan(creal(**(S[s_index].result))))
                {
                    tms_error_handler(EH_SAVE, EXTF_FAILURE, EH_FATAL_ERROR, M->local_expr, S[s_index].subexpr_start);
                    free(arguments);
                    tms_free_arg_list(L);
                    return NAN;
                }
                if (!tms_is_real(**(S[s_index].result)) && M->enable_complex == false)
                {
                    tms_error_handler(EH_SAVE, COMPLEX_DISABLED, EH_NONFATAL_ERROR, NULL);
                    free(arguments);
                    tms_free_arg_list(L);
                    return NAN;
                }

                S[s_index].exec_extf = false;
                free(arguments);
                tms_free_arg_list(L);
            }
            ++s_index;

            continue;
        }
        i_node = S[s_index].nodes + S[s_index].start_node;

        // Case of an expression with one operand
        if (S[s_index].op_count == 0)
            *(i_node->result) = i_node->left_operand;
        // Normal case, iterate through the nodes following the evaluation order.
        else
            while (i_node != NULL)
            {
                // Probably a parsing bug
                if (i_node->result == NULL)
                {
                    tms_error_handler(EH_SAVE, INTERNAL_ERROR, EH_FATAL_ERROR, NULL);
                    return NAN;
                }
                switch (i_node->operator)
                {
                case '+':
                    *(i_node->result) = i_node->left_operand + i_node->right_operand;
                    break;

                case '-':
                    *(i_node->result) = i_node->left_operand - i_node->right_operand;
                    break;

                case '*':
                    *(i_node->result) = i_node->left_operand * i_node->right_operand;
                    break;

                case '/':
                    if (i_node->right_operand == 0)
                    {
                        tms_error_handler(EH_SAVE, DIVISION_BY_ZERO, EH_FATAL_ERROR, M->local_expr, i_node->operator_index);
                        return NAN;
                    }
                    *(i_node->result) = i_node->left_operand / i_node->right_operand;
                    break;

                case '%':
                    if (i_node->right_operand == 0)
                    {
                        tms_error_handler(EH_SAVE, MODULO_ZERO, EH_FATAL_ERROR, M->local_expr, i_node->operator_index);
                        return NAN;
                    }
                    if (cimag(i_node->left_operand) != 0 || cimag(i_node->right_operand) != 0)
                    {
                        tms_error_handler(EH_SAVE, MODULO_COMPLEX_NOT_SUPPORTED, EH_FATAL_ERROR, M->local_expr, i_node->operator_index);
                        return NAN;
                    }
                    else
                        *(i_node->result) = fmod(i_node->left_operand, i_node->right_operand);
                    break;

                case '^':
                    // Use non complex power function if the operands are real
                    if (M->enable_complex == false)
                    {
                        *(i_node->result) = pow(i_node->left_operand, i_node->right_operand);
                        if (isnan(creal(*(i_node->result))))
                        {
                            tms_error_handler(EH_SAVE, MATH_ERROR, EH_NONFATAL_ERROR, M->local_expr, i_node->operator_index);
                            return NAN;
                        }
                    }
                    else
                        *(i_node->result) = tms_cpow(i_node->left_operand, i_node->right_operand);
                    break;
                }
                i_node = i_node->next;
            }

        // Executing function on the subexpression result
        switch (S[s_index].func_type)
        {
        case TMS_F_REAL:
            **(S[s_index].result) = (*(S[s_index].func.real))(**(S[s_index].result));
            break;
        case TMS_F_CMPLX:
            **(S[s_index].result) = (*(S[s_index].func.cmplx))(**(S[s_index].result));
            break;
        case TMS_F_RUNTIME:
            // Copy the function structure, set the unknown and solve
            tms_math_expr *F = tms_dup_mexpr(S[s_index].func.runtime->F);
            tms_set_unknown(F, **(S[s_index].result));
            **(S[s_index].result) = tms_evaluate(F);
            tms_delete_math_expr(F);
        }

        if (isnan((double)**(S[s_index].result)))
        {
            tms_error_handler(EH_SAVE, MATH_ERROR, EH_NONFATAL_ERROR, M->local_expr, S[s_index].solve_start);
            return NAN;
        }
        ++s_index;
    }

    if (M->runvar_i != -1 && !tms_g_vars[M->runvar_i].is_constant)
        tms_g_vars[M->runvar_i].value = M->answer;

    if (_tms_debug)
        tms_dump_expr(M, true);

    return M->answer;
}

int tms_int_evaluate(tms_int_expr *M, int64_t *result)
{
    // No NULL pointer dereference allowed.
    if (M == NULL)
        return -1;

    tms_int_op_node *i_node;
    int s_index = 0, s_count = M->subexpr_count;
    int state;
    // Subexpression pointer to access the subexpression array.
    tms_int_subexpr *S = M->subexpr_ptr;
    while (s_index < s_count)
    {
        // Extended functions have no nodes
        if (S[s_index].nodes == NULL)
        {
            if (S[s_index].exec_extf)
            {
                char *arguments;
                tms_arg_list *L;
                bool _debug_state = _tms_debug;

                int length = S[s_index].solve_end - S[s_index].solve_start + 1;

                // Copy arguments
                arguments = tms_strndup(M->local_expr + S[s_index].solve_start, length);
                L = tms_get_args(arguments);

                // Disable debug output for extended functions
                _tms_debug = false;

                // Call the extended function
                state = ((*(S[s_index].func.extended))(L, *(S[s_index].result))) & tms_int_mask;

                _tms_debug = _debug_state;
                if (state == -1)
                {
                    tms_error_handler(EH_SAVE, EXTF_FAILURE, EH_FATAL_ERROR, M->local_expr, S[s_index].subexpr_start);
                    free(arguments);
                    tms_free_arg_list(L);
                    return -1;
                }
                else
                {
                    S[s_index].exec_extf = false;
                    **(S[s_index].result) &= tms_int_mask;
                    free(arguments);
                    tms_free_arg_list(L);
                }
            }
            ++s_index;

            continue;
        }
        i_node = S[s_index].nodes + S[s_index].start_node;

        // Case of an expression with one operand
        if (S[s_index].op_count == 0)
            *(i_node->result) = i_node->left_operand;
        // Normal case, iterate through the nodes following the evaluation order.
        else
            while (i_node != NULL)
            {
                // Probably a parsing bug
                if (i_node->result == NULL)
                {
                    tms_error_handler(EH_SAVE, INTERNAL_ERROR, EH_FATAL_ERROR, NULL);
                    return -1;
                }
                switch (i_node->operator)
                {
                case '&':
                    *(i_node->result) = i_node->left_operand & i_node->right_operand;
                    break;

                case '|':
                    *(i_node->result) = i_node->left_operand | i_node->right_operand;
                    break;

                case '^':
                    *(i_node->result) = i_node->left_operand ^ i_node->right_operand;
                    break;

                case '+':
                    *(i_node->result) = i_node->left_operand + i_node->right_operand;
                    break;

                case '-':
                    *(i_node->result) = i_node->left_operand - i_node->right_operand;
                    break;

                case '*':
                    *(i_node->result) = i_node->left_operand * i_node->right_operand;
                    break;

                case '/':
                    if (i_node->right_operand == 0)
                    {
                        tms_error_handler(EH_SAVE, DIVISION_BY_ZERO, EH_FATAL_ERROR, M->local_expr, i_node->operator_index);
                        return -1;
                    }
                    *(i_node->result) = i_node->left_operand / i_node->right_operand;
                    break;

                case '%':
                    if (i_node->right_operand == 0)
                    {
                        tms_error_handler(EH_SAVE, MODULO_ZERO, EH_FATAL_ERROR, M->local_expr, i_node->operator_index);
                        return -1;
                    }
                    else
                        *(i_node->result) = i_node->left_operand % i_node->right_operand;
                    break;
                }
                *(i_node->result) = *(i_node->result) & tms_int_mask;
                i_node = i_node->next;
            }

        // Executing function on the subexpression result
        switch (S[s_index].func_type)
        {
        case TMS_F_REAL:
            state = (*(S[s_index].func.simple))(**(S[s_index].result), *(S[s_index].result));
            if (state == -1)
                return -1;

            break;
        }
        ++s_index;
    }

    if (M->runvar_i != -1)
        tms_g_int_vars[M->runvar_i].value = tms_sign_extend(M->answer);

    if (_tms_debug)
        tms_dump_int_expr(M, true);

    *result = M->answer;
    return 0;
}

void _tms_set_unknowns_data(tms_math_expr *M)
{
    int i = 0, s_index, buffer_size = 16;
    tms_unknown_operand *x_data = malloc(buffer_size * sizeof(tms_unknown_operand));
    tms_math_subexpr *subexpr_ptr = M->subexpr_ptr;
    tms_op_node *i_node;

    for (s_index = 0; s_index < M->subexpr_count; ++s_index)
    {
        if (subexpr_ptr[s_index].nodes == NULL)
            continue;

        i_node = subexpr_ptr[s_index].nodes + subexpr_ptr[s_index].start_node;
        while (i_node != NULL)
        {
            if (i == buffer_size)
            {
                buffer_size *= 2;
                x_data = realloc(x_data, buffer_size * sizeof(tms_unknown_operand));
            }
            // Case of unknown left operand
            if (i_node->unknowns_data & UNK_LEFT)
            {
                x_data[i].is_negative = i_node->unknowns_data & UNK_LNEG;
                x_data[i].unknown_ptr = &(i_node->left_operand);
                i_node->left_operand = 0;
                ++i;
            }
            // Case of a unknown right operand
            if (i_node->unknowns_data & UNK_RIGHT)
            {
                x_data[i].is_negative = i_node->unknowns_data & UNK_RNEG;
                x_data[i].unknown_ptr = &(i_node->right_operand);
                i_node->right_operand = 0;
                ++i;
            }
            i_node = i_node->next;
        }
    }
    if (i != 0)
    {
        x_data = realloc(x_data, i * sizeof(tms_unknown_operand));
        M->unknown_count = i;
        M->x_data = x_data;
    }
    else
        free(x_data);
}

void tms_set_unknown(tms_math_expr *M, double complex value)
{
    int i;
    for (i = 0; i < M->unknown_count; ++i)
    {
        if (M->x_data[i].is_negative)
            *(M->x_data[i].unknown_ptr) = -value;
        else
            *(M->x_data[i].unknown_ptr) = value;
    }
}

bool _print_operand_source(tms_math_subexpr *S, double complex *operand, int s_index, bool was_evaluated)
{
    int n;
    while (s_index != -1)
    {
        // Scan all nodes in the current and previous subexpressions for one that points at this operand
        for (n = 0; n < S[s_index].op_count; ++n)
        {
            if (S[s_index].nodes == NULL)
                continue;
            if (operand == S[s_index].nodes[n].result)
            {
                printf("[S%d;N%d]", s_index, n);
                if (was_evaluated)
                {
                    printf(" = ");
                    tms_print_value(*operand);
                }
                return true;
            }
        }
        if (operand == *(S[s_index].result))
        {
            printf("[S:%d]", s_index);
            if (was_evaluated)
            {
                printf(" = ");
                tms_print_value(*operand);
            }
            return true;
        }
        --s_index;
    }
    return false;
}

void tms_dump_expr(tms_math_expr *M, bool was_evaluated)
{
    int s_index = 0;
    int s_count = M->subexpr_count;
    tms_math_subexpr *S = M->subexpr_ptr;
    char *tmp = NULL;
    tms_op_node *tmp_node;
    puts("Dumping expression data:\n");
    while (s_index < s_count)
    {
        // Find the name of the function to execute
        switch (S[s_index].func_type)
        {
        case TMS_F_REAL:
        case TMS_F_CMPLX:
        case TMS_F_EXTENDED:
            tmp = _tms_lookup_function_name(S[s_index].func.real, S[s_index].func_type);
            break;

        case TMS_F_RUNTIME:
            tmp = S[s_index].func.runtime->name;
            break;

        default:
            tmp = NULL;
        }

        printf("subexpr %d:\nftype = %u, fname = %s, depth = %d\n",
               s_index, S[s_index].func_type, tmp, S[s_index].depth);

        // Lookup result pointer location
        for (int i = 0; i < s_count; ++i)
        {
            for (int j = 0; j < S[i].op_count; ++j)
            {
                if (S[i].nodes != NULL && S[i].nodes[j].result == *(S[s_index].result))
                {
                    printf("result at subexpr %d, node %d\n\n", i, j);
                    break;
                }
            }
            // Case of a no op expression
            if (S[i].op_count == 0 && S[i].nodes != NULL && S[i].nodes[0].result == *(S[s_index].result))
                printf("result at subexpr %d, node 0\n\n", i);
        }

        if (S[s_index].nodes == NULL)
            puts("Expression has no nodes");
        else
        {
            tmp_node = S[s_index].nodes + S[s_index].start_node;

            // Dump nodes data
            while (tmp_node != NULL)
            {
                printf("[%d]: ", tmp_node->node_index);
                printf("( ");

                if (_print_operand_source(S, &(tmp_node->left_operand), s_index, was_evaluated) == false)
                    tms_print_value(tmp_node->left_operand);

                printf(" )");

                // No one wants to see uninitialized values (skip nodes used to hold one number)
                if (S[s_index].op_count != 0)
                {
                    printf(" %c ", tmp_node->operator);
                    printf("( ");

                    if (_print_operand_source(S, &(tmp_node->right_operand), s_index, was_evaluated) == false)
                        tms_print_value(tmp_node->right_operand);

                    printf(" )");
                }

                if (tmp_node->next != NULL)
                {
                    int i;
                    char left_or_right = '\0';
                    for (i = 0; i < S[s_index].op_count; ++i)
                    {
                        if (&(S[s_index].nodes[i].right_operand) == tmp_node->result)
                        {
                            left_or_right = 'R';
                            break;
                        }
                        else if (&(S[s_index].nodes[i].left_operand) == tmp_node->result)
                        {
                            left_or_right = 'L';
                            break;
                        }

                        if (S[s_index].op_count == 0 && &(S[s_index].nodes->right_operand) == tmp_node->result)
                        {
                            i = 0;
                            left_or_right = 'R';
                        }
                    }

                    printf(" a: %d%c --> ", i, left_or_right);
                }
                else
                    printf(" --> ");
                printf("\n");
                tmp_node = tmp_node->next;
            }
        }
        ++s_index;
        puts("end\n");
    }
}

bool _print_int_operand_source(tms_int_subexpr *S, int64_t *operand, int s_index, bool was_evaluated)
{
    int n;
    while (s_index != -1)
    {
        // Scan all nodes in the current and previous subexpressions for one that points at this operand
        for (n = 0; n < S[s_index].op_count; ++n)
        {
            if (S[s_index].nodes == NULL)
                continue;
            if (operand == S[s_index].nodes[n].result)
            {
                printf("[S%d;N%d]", s_index, n);
                if (was_evaluated)
                {
                    printf(" = ");
                    tms_print_hex(*operand);
                }
                return true;
            }
        }
        if (operand == *(S[s_index].result))
        {
            printf("[S:%d]", s_index);
            if (was_evaluated)
            {
                printf(" = ");
                tms_print_hex(*operand);
            }
            return true;
        }
        --s_index;
    }
    return false;
}

void tms_dump_int_expr(tms_int_expr *M, bool was_evaluated)
{
    int s_index = 0;
    int s_count = M->subexpr_count;
    tms_int_subexpr *S = M->subexpr_ptr;
    char *tmp = NULL;
    tms_int_op_node *tmp_node;
    puts("Dumping expression data:\n");
    while (s_index < s_count)
    {
        // Find the name of the function to execute
        switch (S[s_index].func_type)
        {
        case TMS_F_REAL:
        case TMS_F_EXTENDED:
            tmp = _tms_lookup_int_function_name(S[s_index].func.simple, S[s_index].func_type);
            break;

        default:
            tmp = NULL;
        }

        printf("subexpr %d:\nftype = %u, fname = %s, depth = %d\n",
               s_index, S[s_index].func_type, tmp, S[s_index].depth);

        // Lookup result pointer location
        for (int i = 0; i < s_count; ++i)
        {
            for (int j = 0; j < S[i].op_count; ++j)
            {
                if (S[i].nodes != NULL && S[i].nodes[j].result == *(S[s_index].result))
                {
                    printf("result at subexpr %d, node %d\n\n", i, j);
                    break;
                }
            }
            // Case of a no op expression
            if (S[i].op_count == 0 && S[i].nodes != NULL && S[i].nodes[0].result == *(S[s_index].result))
                printf("result at subexpr %d, node 0\n\n", i);
        }

        if (S[s_index].nodes == NULL)
            puts("Expression has no nodes");
        else
        {
            tmp_node = S[s_index].nodes + S[s_index].start_node;

            // Dump nodes data
            while (tmp_node != NULL)
            {
                printf("[%d]: ", tmp_node->node_index);
                printf("( ");

                if (_print_int_operand_source(S, &(tmp_node->left_operand), s_index, was_evaluated) == false)
                    tms_print_hex(tmp_node->left_operand);

                printf(" )");
                // No one wants to see uninitialized values (skip nodes used to hold one number)
                if (S[s_index].op_count != 0)
                {
                    printf(" %c ", tmp_node->operator);
                    printf("( ");

                    if (_print_int_operand_source(S, &(tmp_node->right_operand), s_index, was_evaluated) == false)
                        tms_print_hex(tmp_node->right_operand);

                    printf(" )");
                }

                if (tmp_node->next != NULL)
                {
                    int i;
                    char left_or_right = '\0';
                    for (i = 0; i < S[s_index].op_count; ++i)
                    {
                        if (&(S[s_index].nodes[i].right_operand) == tmp_node->result)
                        {
                            left_or_right = 'R';
                            break;
                        }
                        else if (&(S[s_index].nodes[i].left_operand) == tmp_node->result)
                        {
                            left_or_right = 'L';
                            break;
                        }

                        if (S[s_index].op_count == 0 && &(S[s_index].nodes->right_operand) == tmp_node->result)
                        {
                            i = 0;
                            left_or_right = 'R';
                        }
                    }

                    printf(" a: %d%c --> ", i, left_or_right);
                }
                else
                    printf(" --> ");
                printf("\n");
                tmp_node = tmp_node->next;
            }
        }
        ++s_index;
        puts("end\n");
    }
}
