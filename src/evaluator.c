/*
Copyright (C) 2023-2024 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "evaluator.h"
#include "bitwise.h"
#include "error_handler.h"
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

double complex _tms_evaluate_unsafe(tms_math_expr *M);

double complex tms_evaluate(tms_math_expr *M, int options)
{
    if ((options & NO_LOCK) != 1)
        tms_lock_evaluator(TMS_EVALUATOR);

    if (tms_get_error_count(TMS_EVALUATOR | TMS_PARSER, EH_ALL_ERRORS) != 0)
    {
        fputs(ERROR_DB_NOT_EMPTY, stderr);
        tms_clear_errors(TMS_EVALUATOR | TMS_PARSER);
    }

    double complex result = _tms_evaluate_unsafe(M);

    if (isnan(creal(result)) && (options & NO_PRINT) == 0)
        tms_print_errors(TMS_EVALUATOR | TMS_PARSER);

    if ((options & NO_LOCK) != 1)
        tms_unlock_evaluator(TMS_EVALUATOR);
    return result;
}

double complex *tms_solve_list(tms_arg_list *expr_list, int options, tms_arg_list *labels)
{
    if (expr_list->count < 1)
        return NULL;
    double complex *answer_list = malloc(expr_list->count * sizeof(double complex));

    for (int i = 0; i < expr_list->count; ++i)
    {
        answer_list[i] = tms_solve_e(expr_list->arguments[i], options, labels);
        if (isnan(creal(answer_list[i])))
        {
            free(answer_list);
            answer_list = NULL;
            break;
        }
    }

    return answer_list;
}

int64_t *tms_int_solve_list(tms_arg_list *expr_list, tms_arg_list *labels)
{
    if (expr_list->count < 1)
        return NULL;
    int64_t *answer_list = malloc(expr_list->count * sizeof(int64_t));
    int status;
    for (int i = 0; i < expr_list->count; ++i)
    {
        status = tms_int_solve_e(expr_list->arguments[i], answer_list + i, NO_LOCK, labels);
        if (status != 0)
        {
            free(answer_list);
            answer_list = NULL;
            break;
        }
    }

    return answer_list;
}

double complex _tms_evaluate_unsafe(tms_math_expr *M)
{
    if (M == NULL)
        return NAN;
    tms_op_node *i_node;
    int i;

    tms_math_subexpr *S = M->S;
    for (i = 0; i < M->subexpr_count; ++i)
    {
        // Extended and User functions have no nodes
        if (S[i].nodes == NULL)
        {
            if (S[i].func_type == TMS_F_EXTENDED && S[i].exec_extf)
            {
                bool _debug_state = _tms_debug;

                // Disable debug output for extended functions
                _tms_debug = false;

                // Call the extended function using its pointer
                int status = (*(S[i].func.extended))(S[i].L, M->labels, *(S[i].result));

                _tms_debug = _debug_state;

                if (status != 0)
                {
                    // If the function didn't generate an error itself, provide a generic one
                    if (tms_get_error_count(TMS_EVALUATOR | TMS_PARSER, EH_ALL_ERRORS) == 0)
                        tms_save_error(TMS_EVALUATOR, EXTF_FAILURE, EH_FATAL, M->expr, S[i].subexpr_start);
                    else
                        tms_modify_last_error(TMS_EVALUATOR | TMS_PARSER, M->expr, S[i].subexpr_start,
                                              "In function args: ");

                    return NAN;
                }
                if (!tms_is_real(**(S[i].result)) && M->enable_complex == false)
                {
                    tms_save_error(TMS_EVALUATOR, COMPLEX_DISABLED, EH_NONFATAL, NULL, 0);
                    return NAN;
                }
                S[i].exec_extf = false;
            }
            else if (S[i].func_type == TMS_F_USER)
            {
                const tms_ufunc *userf = tms_get_ufunc_by_name(S[i].func.user);
                if (userf == NULL)
                {
                    tms_save_error(TMS_EVALUATOR, USER_FUNCTION_NOT_FOUND, EH_FATAL, M->expr, M->S[i].subexpr_start);
                    return NAN;
                }
                if (!_tms_validate_args_count(userf->F->labels->count, S[i].L->count, TMS_EVALUATOR))
                {
                    tms_modify_last_error(TMS_EVALUATOR, M->expr, M->S[i].subexpr_start, NULL);
                    return NAN;
                }
                double complex *arguments = tms_solve_list(S[i].L, NO_LOCK, M->labels);
                if (arguments == NULL)
                {
                    return NAN;
                }
                tms_math_expr *F = tms_dup_mexpr(userf->F);
                F->labels->payload = arguments;
                F->labels->payload_size = S[i].L->count * sizeof(double complex);
                // Set the label values (passed as arguments earlier)
                tms_set_labels_values(F, arguments);
                **(S[i].result) = _tms_evaluate_unsafe(F);
                tms_delete_math_expr(F);
            }
        }
        else
        {
            i_node = S[i].nodes + S[i].start_node;

            // Case of an expression with one operand
            if (S[i].op_count == 0)
                *(i_node->result) = i_node->left_operand;
            // Normal case, iterate through the nodes following the evaluation order.
            else
                while (i_node != NULL)
                {
                    // Probably a parsing bug
                    if (i_node->result == NULL)
                    {
                        tms_save_error(TMS_EVALUATOR, INTERNAL_ERROR, EH_FATAL, NULL, 0);
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
                            tms_save_error(TMS_EVALUATOR, DIVISION_BY_ZERO, EH_FATAL, M->expr, i_node->operator_index);
                            return NAN;
                        }
                        *(i_node->result) = i_node->left_operand / i_node->right_operand;
                        break;

                    case '%':
                        if (i_node->right_operand == 0)
                        {
                            tms_save_error(TMS_EVALUATOR, MODULO_ZERO, EH_FATAL, M->expr, i_node->operator_index);
                            return NAN;
                        }
                        if (cimag(i_node->left_operand) != 0 || cimag(i_node->right_operand) != 0)
                        {
                            tms_save_error(TMS_EVALUATOR, MODULO_COMPLEX_NOT_SUPPORTED, EH_FATAL, M->expr,
                                           i_node->operator_index);
                            return NAN;
                        }
                        else
                        {
                            *(i_node->result) = fmod(i_node->left_operand, i_node->right_operand);
                            if (isnan(creal(*(i_node->result))))
                            {
                                tms_save_error(TMS_EVALUATOR, MATH_ERROR, EH_NONFATAL, M->expr, i_node->operator_index);
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
                                tms_save_error(TMS_EVALUATOR, MATH_ERROR, EH_NONFATAL, M->expr, i_node->operator_index);
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
            switch (S[i].func_type)
            {
            case TMS_F_REAL:
                **(S[i].result) = (*(S[i].func.real))(**(S[i].result));
                break;
            case TMS_F_CMPLX:
                **(S[i].result) = (*(S[i].func.cmplx))(**(S[i].result));
                break;
            }

            if (isnan((double)**(S[i].result)))
            {
                tms_save_error(TMS_EVALUATOR, MATH_ERROR, EH_NONFATAL, M->expr, S[i].subexpr_start);
                return NAN;
            }
        }
    }

    if (_tms_debug)
        tms_dump_expr(M, true);

    return M->answer;
}

int _tms_int_evaluate_unsafe(tms_int_expr *M, int64_t *result)
{
    // No NULL pointer dereference allowed.
    if (M == NULL)
        return -1;

    tms_int_op_node *i_node;
    int state;
    // Subexpression pointer to access the subexpression array.
    tms_int_subexpr *S = M->S;
    for (int i = 0; i < M->subexpr_count; ++i)
    {
        // Extended functions have no nodes
        if (S[i].nodes == NULL)
        {
            if (S[i].func_type == TMS_F_INT_EXTENDED && S[i].exec_extf)
            {
                bool _debug_state = _tms_debug;

                // Disable debug output for extended functions
                _tms_debug = false;

                // Call the extended function using its pointer
                int status = (*(S[i].func.extended))(S[i].L, M->labels, *(S[i].result));

                _tms_debug = _debug_state;

                if (status != 0)
                {
                    // If the function didn't generate an error itself, provide a generic one
                    if (tms_get_error_count(TMS_INT_EVALUATOR | TMS_INT_PARSER, EH_ALL_ERRORS) == 0)
                        tms_save_error(TMS_INT_EVALUATOR, EXTF_FAILURE, EH_FATAL, M->expr, S[i].subexpr_start);
                    else
                        tms_modify_last_error(TMS_INT_EVALUATOR | TMS_INT_PARSER, M->expr, S[i].subexpr_start,
                                              "In function args: ");

                    return -1;
                }
                S[i].exec_extf = false;
            }
            else if (S[i].func_type == TMS_F_INT_USER)
            {
                const tms_int_ufunc *userf = tms_get_int_ufunc_by_name(S[i].func.user);
                if (userf == NULL)
                {
                    tms_save_error(TMS_INT_EVALUATOR, USER_FUNCTION_NOT_FOUND, EH_FATAL, M->expr,
                                   M->S[i].subexpr_start);
                    return -1;
                }
                if (!_tms_validate_args_count(userf->F->labels->count, S[i].L->count, TMS_INT_EVALUATOR))
                {
                    tms_modify_last_error(TMS_INT_EVALUATOR, M->expr, M->S[i].subexpr_start, NULL);
                    return -1;
                }
                int64_t *arguments = tms_int_solve_list(S[i].L, M->labels);
                if (arguments == NULL)
                {
                    return -1;
                }
                tms_int_expr *F = tms_dup_int_expr(userf->F);
                F->labels->payload = arguments;
                F->labels->payload_size = S[i].L->count * sizeof(int64_t);
                // Set the label values (passed as arguments earlier)
                tms_set_int_labels_values(F, arguments);
                _tms_int_evaluate_unsafe(F, *(S[i].result));
                tms_delete_int_expr(F);
            }
            **(S[i].result) &= tms_int_mask;
        }
        else
        {
            i_node = S[i].nodes + S[i].start_node;

            // Case of an expression with one operand
            if (S[i].op_count == 0)
                *(i_node->result) = i_node->left_operand;
            // Normal case, iterate through the nodes following the evaluation order.
            else
                while (i_node != NULL)
                {
                    // Probably a parsing bug
                    if (i_node->result == NULL)
                    {
                        tms_save_error(TMS_INT_EVALUATOR, INTERNAL_ERROR, EH_FATAL, NULL, 0);
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
                            tms_save_error(TMS_INT_EVALUATOR, DIVISION_BY_ZERO, EH_FATAL, M->expr,
                                           i_node->operator_index);
                            return -1;
                        }
                        *(i_node->result) = i_node->left_operand / i_node->right_operand;
                        break;

                    case '%':
                        if (i_node->right_operand == 0)
                        {
                            tms_save_error(TMS_INT_EVALUATOR, MODULO_ZERO, EH_FATAL, M->expr, i_node->operator_index);
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
            switch (S[i].func_type)
            {
            case TMS_F_INT64:
                state = (*(S[i].func.simple))(tms_sign_extend(**(S[i].result)), *(S[i].result));
                **(S[i].result) &= tms_int_mask;
                if (state == -1)
                {
                    // If the function didn't generate an error itself, provide a generic one
                    if (tms_get_error_count(TMS_INT_EVALUATOR, EH_ALL_ERRORS) == 0)
                        tms_save_error(TMS_INT_EVALUATOR, UNKNOWN_FUNC_ERROR, EH_FATAL, M->expr, S[i].subexpr_start);
                    else
                        // No need to include the flag for INT_PARSER since regular functions will never call the parser
                        tms_modify_last_error(TMS_INT_EVALUATOR, M->expr, S[i].subexpr_start, NULL);
                    return -1;
                }

                break;
            }
        }
    }

    if (_tms_debug)
        tms_dump_int_expr(M, true);

    *result = M->answer;
    return 0;
}

int tms_int_evaluate(tms_int_expr *M, int64_t *result, int options)
{
    if ((options & NO_LOCK) != 1)
        tms_lock_evaluator(TMS_INT_EVALUATOR);

    if (tms_get_error_count(TMS_INT_EVALUATOR | TMS_INT_PARSER, EH_ALL_ERRORS) != 0)
    {
        fputs(ERROR_DB_NOT_EMPTY, stderr);
        tms_clear_errors(TMS_INT_EVALUATOR | TMS_INT_PARSER);
    }

    int exit_status = _tms_int_evaluate_unsafe(M, result);
    if (exit_status != 0 && (options & NO_PRINT) == 0)
        tms_print_errors(TMS_INT_EVALUATOR | TMS_INT_PARSER);

    if ((options & NO_LOCK) != 1)
        tms_unlock_evaluator(TMS_INT_EVALUATOR);
    return exit_status;
}

void tms_set_labels_values(tms_math_expr *M, double complex *values_list)
{
    int i;
    for (i = 0; i < M->labeled_operands_count; ++i)
    {
        if (M->all_labeled_ops[i].is_negative)
            *(double complex *)(M->all_labeled_ops[i].ptr) = -values_list[M->all_labeled_ops[i].id];
        else
            *(double complex *)(M->all_labeled_ops[i].ptr) = values_list[M->all_labeled_ops[i].id];
    }
}

void tms_set_int_labels_values(tms_int_expr *M, int64_t *values_list)
{
    int i;
    for (i = 0; i < M->labeled_operands_count; ++i)
    {
        if (M->all_labeled_ops[i].is_negative)
            *(int64_t *)(M->all_labeled_ops[i].ptr) = -values_list[M->all_labeled_ops[i].id];
        else
            *(int64_t *)(M->all_labeled_ops[i].ptr) = values_list[M->all_labeled_ops[i].id];
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
            tmp = tms_get_name(M->expr, S[s_i].subexpr_start, true);
            break;

        case TMS_F_USER:
            tmp = strdup(S[s_i].func.user);
            break;

        default:
            tmp = NULL;
        }

        printf("subexpr %d:\nftype = %u, fname = %s, depth = %d\n", s_i, S[s_i].func_type, tmp, S[s_i].depth);
        free(tmp);

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
        case TMS_F_INT_EXTENDED:
            tmp = tms_get_name(M->expr, S[s_i].subexpr_start, true);
            break;

        case TMS_F_INT_USER:
            tmp = strdup(S[s_i].func.user);

        default:
            tmp = NULL;
        }

        printf("subexpr %d:\nftype = %u, fname = %s, depth = %d\n", s_i, S[s_i].func_type, tmp, S[s_i].depth);
        free(tmp);

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
