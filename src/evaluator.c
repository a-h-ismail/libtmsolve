/*
Copyright (C) 2023-2024 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "evaluator.h"
#include "bitwise.h"
#include "int_parser.h"
#include "internals.h"
#include "parser.h"
#include "scientific.h"
#include "string_tools.h"
#include "tms_complex.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

double complex tms_evaluate(tms_math_expr *M)
{
    tms_lock_evaluator(TMS_EVALUATOR);

    if (tms_error_handler(EH_ERROR_COUNT, TMS_EVALUATOR | TMS_PARSER, EH_ALL_ERRORS) != 0)
    {
        fputs(ERROR_DB_NOT_EMPTY, stderr);
        tms_error_handler(EH_CLEAR, TMS_EVALUATOR | TMS_PARSER);
    }

    double complex result = _tms_evaluate_unsafe(M);

    if (isnan(creal(result)))
        tms_error_handler(EH_PRINT, TMS_EVALUATOR | TMS_PARSER);

    tms_unlock_evaluator(TMS_EVALUATOR);
    return result;
}

double complex _tms_evaluate_unsafe(tms_math_expr *M)
{
    // No NULL pointer dereference allowed.
    if (M == NULL)
        return NAN;
    tms_op_node *i_node;
    int s_i = 0, s_count = M->subexpr_count;
    // Subexpression pointer to access the subexpression array.
    tms_math_subexpr *S = M->S;
    while (s_i < s_count)
    {
        // Extended functions have no nodes
        if (S[s_i].nodes == NULL)
        {
            if (S[s_i].exec_extf)
            {
                char *arguments;
                bool _debug_state = _tms_debug;
                tms_arg_list *L;

                int length = S[s_i].solve_end - S[s_i].solve_start + 1;

                // Copy arguments
                arguments = tms_strndup(M->local_expr + S[s_i].solve_start, length);
                L = tms_get_args(arguments);

                // Disable debug output for extended functions
                _tms_debug = false;

                // Call the extended function using its pointer
                **(S[s_i].result) = (*(S[s_i].func.extended))(L);

                _tms_debug = _debug_state;

                if (isnan(creal(**(S[s_i].result))))
                {
                    // If the function didn't generate an error itself, provide a generic one
                    if (tms_error_handler(EH_ERROR_COUNT, TMS_EVALUATOR | TMS_PARSER, EH_ALL_ERRORS) == 0)
                        tms_error_handler(EH_SAVE, TMS_EVALUATOR, EXTF_FAILURE, EH_FATAL, M->local_expr,
                                          S[s_i].subexpr_start);
                    else
                        tms_error_handler(EH_MODIFY, TMS_EVALUATOR | TMS_PARSER, M->local_expr, S[s_i].subexpr_start,
                                          "In function args: ");

                    free(arguments);
                    tms_free_arg_list(L);
                    return NAN;
                }
                if (!tms_is_real(**(S[s_i].result)) && M->enable_complex == false)
                {
                    tms_error_handler(EH_SAVE, TMS_EVALUATOR, COMPLEX_DISABLED, EH_NONFATAL, NULL);
                    free(arguments);
                    tms_free_arg_list(L);
                    return NAN;
                }

                S[s_i].exec_extf = false;
                free(arguments);
                tms_free_arg_list(L);
            }
            ++s_i;

            continue;
        }
        i_node = S[s_i].nodes + S[s_i].start_node;

        // Case of an expression with one operand
        if (S[s_i].op_count == 0)
            *(i_node->result) = i_node->left_operand;
        // Normal case, iterate through the nodes following the evaluation order.
        else
            while (i_node != NULL)
            {
                // Probably a parsing bug
                if (i_node->result == NULL)
                {
                    tms_error_handler(EH_SAVE, TMS_EVALUATOR, INTERNAL_ERROR, EH_FATAL, NULL);
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
                        tms_error_handler(EH_SAVE, TMS_EVALUATOR, DIVISION_BY_ZERO, EH_FATAL, M->local_expr,
                                          i_node->operator_index);
                        return NAN;
                    }
                    *(i_node->result) = i_node->left_operand / i_node->right_operand;
                    break;

                case '%':
                    if (i_node->right_operand == 0)
                    {
                        tms_error_handler(EH_SAVE, TMS_EVALUATOR, MODULO_ZERO, EH_FATAL, M->local_expr,
                                          i_node->operator_index);
                        return NAN;
                    }
                    if (cimag(i_node->left_operand) != 0 || cimag(i_node->right_operand) != 0)
                    {
                        tms_error_handler(EH_SAVE, TMS_EVALUATOR, MODULO_COMPLEX_NOT_SUPPORTED, EH_FATAL, M->local_expr,
                                          i_node->operator_index);
                        return NAN;
                    }
                    else
                    {
                        *(i_node->result) = fmod(i_node->left_operand, i_node->right_operand);
                        if (isnan(creal(*(i_node->result))))
                        {
                            tms_error_handler(EH_SAVE, TMS_EVALUATOR, MATH_ERROR, EH_NONFATAL, M->local_expr,
                                              i_node->operator_index);
                            return NAN;
                        }
                    }
                    break;

                case '^':
                    // Use non complex power function if the operands are real
                    if (M->enable_complex == false)
                    {
                        *(i_node->result) = pow(i_node->left_operand, i_node->right_operand);
                        if (isnan(creal(*(i_node->result))))
                        {
                            tms_error_handler(EH_SAVE, TMS_EVALUATOR, MATH_ERROR, EH_NONFATAL, M->local_expr,
                                              i_node->operator_index);
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
        switch (S[s_i].func_type)
        {
        case TMS_F_REAL:
            **(S[s_i].result) = (*(S[s_i].func.real))(**(S[s_i].result));
            break;
        case TMS_F_CMPLX:
            **(S[s_i].result) = (*(S[s_i].func.cmplx))(**(S[s_i].result));
            break;
        case TMS_F_RUNTIME:
            // Copy the function structure, set the unknown and solve
            tms_math_expr *F = tms_dup_mexpr(S[s_i].func.runtime->F);
            tms_set_unknown(F, **(S[s_i].result));
            **(S[s_i].result) = _tms_evaluate_unsafe(F);
            tms_error_handler(EH_CLEAR, TMS_EVALUATOR);
            tms_delete_math_expr(F);
        }

        if (isnan((double)**(S[s_i].result)))
        {
            tms_error_handler(EH_SAVE, TMS_EVALUATOR, MATH_ERROR, EH_NONFATAL, M->local_expr, S[s_i].subexpr_start);
            return NAN;
        }
        ++s_i;
    }

    if (M->runvar_i != -1 && !tms_g_vars[M->runvar_i].is_constant)
        tms_g_vars[M->runvar_i].value = M->answer;

    if (_tms_debug)
        tms_dump_expr(M, true);

    return M->answer;
}

int tms_int_evaluate(tms_int_expr *M, int64_t *result)
{
    tms_lock_evaluator(TMS_INT_EVALUATOR);

    if (tms_error_handler(EH_ERROR_COUNT, TMS_INT_EVALUATOR | TMS_INT_PARSER, EH_ALL_ERRORS) != 0)
    {
        fputs(ERROR_DB_NOT_EMPTY, stderr);
        tms_error_handler(EH_CLEAR, TMS_INT_EVALUATOR | TMS_INT_PARSER);
    }

    int exit_status = _tms_int_evaluate_unsafe(M, result);
    if (exit_status != 0)
        tms_error_handler(EH_PRINT, TMS_INT_EVALUATOR | TMS_INT_PARSER);

    tms_unlock_evaluator(TMS_INT_EVALUATOR);
    return exit_status;
}

int _tms_int_evaluate_unsafe(tms_int_expr *M, int64_t *result)
{
    // No NULL pointer dereference allowed.
    if (M == NULL)
        return -1;

    tms_int_op_node *i_node;
    int s_i = 0, s_count = M->subexpr_count;
    int state;
    // Subexpression pointer to access the subexpression array.
    tms_int_subexpr *S = M->S;
    while (s_i < s_count)
    {
        // Extended functions have no nodes
        if (S[s_i].nodes == NULL)
        {
            if (S[s_i].exec_extf)
            {
                char *arguments;
                tms_arg_list *L;
                bool _debug_state = _tms_debug;

                int length = S[s_i].solve_end - S[s_i].solve_start + 1;

                // Copy arguments
                arguments = tms_strndup(M->local_expr + S[s_i].solve_start, length);
                L = tms_get_args(arguments);

                // Disable debug output for extended functions
                _tms_debug = false;

                // Call the extended function
                state = ((*(S[s_i].func.extended))(L, *(S[s_i].result)));
                **(S[s_i].result) &= tms_int_mask;

                _tms_debug = _debug_state;
                if (state == -1)
                {
                    // If the function didn't generate an error itself, provide a generic one
                    if (tms_error_handler(EH_ERROR_COUNT, TMS_INT_EVALUATOR | TMS_INT_PARSER, EH_ALL_ERRORS) == 0)
                        tms_error_handler(EH_SAVE, TMS_INT_EVALUATOR, EXTF_FAILURE, EH_FATAL, M->local_expr,
                                          S[s_i].subexpr_start);
                    else
                        tms_error_handler(EH_MODIFY, TMS_INT_EVALUATOR | TMS_INT_PARSER, M->local_expr,
                                          S[s_i].subexpr_start, "In function args: ");

                    free(arguments);
                    tms_free_arg_list(L);
                    return -1;
                }
                else
                {
                    S[s_i].exec_extf = false;
                    **(S[s_i].result) &= tms_int_mask;
                    free(arguments);
                    tms_free_arg_list(L);
                }
            }
            ++s_i;

            continue;
        }
        i_node = S[s_i].nodes + S[s_i].start_node;

        // Case of an expression with one operand
        if (S[s_i].op_count == 0)
            *(i_node->result) = i_node->left_operand;
        // Normal case, iterate through the nodes following the evaluation order.
        else
            while (i_node != NULL)
            {
                // Probably a parsing bug
                if (i_node->result == NULL)
                {
                    tms_error_handler(EH_SAVE, TMS_INT_EVALUATOR, INTERNAL_ERROR, EH_FATAL, NULL);
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
                        tms_error_handler(EH_SAVE, TMS_INT_EVALUATOR, DIVISION_BY_ZERO, EH_FATAL, M->local_expr,
                                          i_node->operator_index);
                        return -1;
                    }
                    *(i_node->result) = i_node->left_operand / i_node->right_operand;
                    break;

                case '%':
                    if (i_node->right_operand == 0)
                    {
                        tms_error_handler(EH_SAVE, TMS_INT_EVALUATOR, MODULO_ZERO, EH_FATAL, M->local_expr,
                                          i_node->operator_index);
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
        switch (S[s_i].func_type)
        {
        case TMS_F_INT64:
            state = (*(S[s_i].func.simple))(tms_sign_extend(**(S[s_i].result)), *(S[s_i].result));
            **(S[s_i].result) &= tms_int_mask;
            if (state == -1)
            {
                // If the function didn't generate an error itself, provide a generic one
                if (tms_error_handler(EH_ERROR_COUNT, TMS_INT_EVALUATOR, EH_ALL_ERRORS) == 0)
                    tms_error_handler(EH_SAVE, TMS_INT_EVALUATOR, UNKNOWN_FUNC_ERROR, EH_FATAL, M->local_expr,
                                      S[s_i].subexpr_start);
                else
                    // No need to include the flag for INT_PARSER since regular functions will never call the parser
                    tms_error_handler(EH_MODIFY, TMS_INT_EVALUATOR, M->local_expr, S[s_i].subexpr_start, NULL);
                return -1;
            }

            break;
        }
        ++s_i;
    }

    if (M->runvar_i != -1)
        tms_g_int_vars[M->runvar_i].value = tms_sign_extend(M->answer);

    if (_tms_debug)
        tms_dump_int_expr(M, true);

    *result = M->answer;
    return 0;
}

void _tms_generate_unknowns_refs(tms_math_expr *M)
{
    int i = 0, s_i, buffer_size = 16;
    tms_unknown_operand *x_data = malloc(buffer_size * sizeof(tms_unknown_operand));
    tms_math_subexpr *subexpr_ptr = M->S;
    tms_op_node *i_node;

    for (s_i = 0; s_i < M->subexpr_count; ++s_i)
    {
        if (subexpr_ptr[s_i].nodes == NULL)
            continue;

        i_node = subexpr_ptr[s_i].nodes + subexpr_ptr[s_i].start_node;
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
        M->unknowns_count = i;
        M->x_data = x_data;
    }
    else
        free(x_data);
}

void tms_set_unknown(tms_math_expr *M, double complex value)
{
    int i;
    for (i = 0; i < M->unknowns_count; ++i)
    {
        if (M->x_data[i].is_negative)
            *(M->x_data[i].unknown_ptr) = -value;
        else
            *(M->x_data[i].unknown_ptr) = value;
    }
}

bool _print_operand_source(tms_math_subexpr *S, double complex *operand, int s_i, bool was_evaluated)
{
    int n;
    while (s_i != -1)
    {
        // Scan all nodes in the current and previous subexpressions for one that points at this operand
        for (n = 0; n < S[s_i].op_count; ++n)
        {
            if (S[s_i].nodes == NULL)
                continue;
            if (operand == S[s_i].nodes[n].result)
            {
                printf("[S%d;N%d]", s_i, n);
                if (was_evaluated)
                {
                    printf(" = ");
                    tms_print_value(*operand);
                }
                return true;
            }
        }
        if (operand == *(S[s_i].result))
        {
            printf("[S:%d]", s_i);
            if (was_evaluated)
            {
                printf(" = ");
                tms_print_value(*operand);
            }
            return true;
        }
        --s_i;
    }
    return false;
}

void tms_dump_expr(tms_math_expr *M, bool was_evaluated)
{
    int s_i = 0;
    int s_count = M->subexpr_count;
    tms_math_subexpr *S = M->S;
    char *tmp = NULL;
    tms_op_node *tmp_node;
    puts("Dumping expression data:\n");
    while (s_i < s_count)
    {
        // Find the name of the function to execute
        switch (S[s_i].func_type)
        {
        case TMS_F_REAL:
        case TMS_F_CMPLX:
        case TMS_F_EXTENDED:
            tmp = _tms_lookup_function_name(S[s_i].func.real, S[s_i].func_type);
            break;

        case TMS_F_RUNTIME:
            tmp = S[s_i].func.runtime->name;
            break;

        default:
            tmp = NULL;
        }

        printf("subexpr %d:\nftype = %u, fname = %s, depth = %d\n", s_i, S[s_i].func_type, tmp, S[s_i].depth);

        // Lookup result pointer location
        for (int i = 0; i < s_count; ++i)
        {
            for (int j = 0; j < S[i].op_count; ++j)
            {
                if (S[i].nodes != NULL && S[i].nodes[j].result == *(S[s_i].result))
                {
                    printf("result at subexpr %d, node %d\n\n", i, j);
                    break;
                }
            }
            // Case of a no op expression
            if (S[i].op_count == 0 && S[i].nodes != NULL && S[i].nodes[0].result == *(S[s_i].result))
                printf("result at subexpr %d, node 0\n\n", i);
        }

        if (S[s_i].nodes == NULL)
            puts("Expression has no nodes");
        else
        {
            tmp_node = S[s_i].nodes + S[s_i].start_node;

            // Dump nodes data
            while (tmp_node != NULL)
            {
                printf("[%d]: ", tmp_node->node_index);
                printf("( ");

                if (_print_operand_source(S, &(tmp_node->left_operand), s_i, was_evaluated) == false)
                    tms_print_value(tmp_node->left_operand);

                printf(" )");

                // No one wants to see uninitialized values (skip nodes used to hold one number)
                if (S[s_i].op_count != 0)
                {
                    printf(" %c ", tmp_node->operator);
                    printf("( ");

                    if (_print_operand_source(S, &(tmp_node->right_operand), s_i, was_evaluated) == false)
                        tms_print_value(tmp_node->right_operand);

                    printf(" )");
                }

                if (tmp_node->next != NULL)
                {
                    int i;
                    char left_or_right = '\0';
                    for (i = 0; i < S[s_i].op_count; ++i)
                    {
                        if (&(S[s_i].nodes[i].right_operand) == tmp_node->result)
                        {
                            left_or_right = 'R';
                            break;
                        }
                        else if (&(S[s_i].nodes[i].left_operand) == tmp_node->result)
                        {
                            left_or_right = 'L';
                            break;
                        }

                        if (S[s_i].op_count == 0 && &(S[s_i].nodes->right_operand) == tmp_node->result)
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
        ++s_i;
        puts("end\n");
    }
}

bool _print_int_operand_source(tms_int_subexpr *S, int64_t *operand, int s_i, bool was_evaluated)
{
    int n;
    while (s_i != -1)
    {
        // Scan all nodes in the current and previous subexpressions for one that points at this operand
        for (n = 0; n < S[s_i].op_count; ++n)
        {
            if (S[s_i].nodes == NULL)
                continue;
            if (operand == S[s_i].nodes[n].result)
            {
                printf("[S%d;N%d]", s_i, n);
                if (was_evaluated)
                {
                    printf(" = ");
                    tms_print_hex(*operand);
                }
                return true;
            }
        }
        if (operand == *(S[s_i].result))
        {
            printf("[S:%d]", s_i);
            if (was_evaluated)
            {
                printf(" = ");
                tms_print_hex(*operand);
            }
            return true;
        }
        --s_i;
    }
    return false;
}

void tms_dump_int_expr(tms_int_expr *M, bool was_evaluated)
{
    int s_i = 0;
    int s_count = M->subexpr_count;
    tms_int_subexpr *S = M->S;
    char *tmp = NULL;
    tms_int_op_node *tmp_node;
    puts("Dumping expression data:\n");
    while (s_i < s_count)
    {
        // Find the name of the function to execute
        switch (S[s_i].func_type)
        {
        case TMS_F_INT64:
            tmp = _tms_lookup_int_function_name(S[s_i].func.simple, TMS_F_INT64);
            break;
        case TMS_F_INT_EXTENDED:
            tmp = _tms_lookup_int_function_name(S[s_i].func.extended, TMS_F_INT_EXTENDED);
            break;

        default:
            tmp = NULL;
        }

        printf("subexpr %d:\nftype = %u, fname = %s, depth = %d\n", s_i, S[s_i].func_type, tmp, S[s_i].depth);

        // Lookup result pointer location
        for (int i = 0; i < s_count; ++i)
        {
            for (int j = 0; j < S[i].op_count; ++j)
            {
                if (S[i].nodes != NULL && S[i].nodes[j].result == *(S[s_i].result))
                {
                    printf("result at subexpr %d, node %d\n\n", i, j);
                    break;
                }
            }
            // Case of a no op expression
            if (S[i].op_count == 0 && S[i].nodes != NULL && S[i].nodes[0].result == *(S[s_i].result))
                printf("result at subexpr %d, node 0\n\n", i);
        }

        if (S[s_i].nodes == NULL)
            puts("Expression has no nodes");
        else
        {
            tmp_node = S[s_i].nodes + S[s_i].start_node;

            // Dump nodes data
            while (tmp_node != NULL)
            {
                printf("[%d]: ", tmp_node->node_index);
                printf("( ");

                if (_print_int_operand_source(S, &(tmp_node->left_operand), s_i, was_evaluated) == false)
                    tms_print_hex(tmp_node->left_operand);

                printf(" )");
                // No one wants to see uninitialized values (skip nodes used to hold one number)
                if (S[s_i].op_count != 0)
                {
                    printf(" %c ", tmp_node->operator);
                    printf("( ");

                    if (_print_int_operand_source(S, &(tmp_node->right_operand), s_i, was_evaluated) == false)
                        tms_print_hex(tmp_node->right_operand);

                    printf(" )");
                }

                if (tmp_node->next != NULL)
                {
                    int i;
                    char left_or_right = '\0';
                    for (i = 0; i < S[s_i].op_count; ++i)
                    {
                        if (&(S[s_i].nodes[i].right_operand) == tmp_node->result)
                        {
                            left_or_right = 'R';
                            break;
                        }
                        else if (&(S[s_i].nodes[i].left_operand) == tmp_node->result)
                        {
                            left_or_right = 'L';
                            break;
                        }

                        if (S[s_i].op_count == 0 && &(S[s_i].nodes->right_operand) == tmp_node->result)
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
        ++s_i;
        puts("end\n");
    }
}
