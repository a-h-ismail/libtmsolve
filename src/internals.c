/*
Copyright (C) 2022-2026 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "hashmap.h"
#include "hashset.h"
#include "libtmsolve.h"
#include "m_errors.h"
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

double complex tms_g_ans = 0;
int64_t tms_g_int_ans = 0;

const tms_rc_func tms_g_rc_func[] = {
    {"fact", tms_fact, tms_cfact}, {"abs", fabs, cabs_z},        {"exp", exp, tms_cexp},
    {"ceil", ceil, tms_cceil},     {"floor", floor, tms_cfloor}, {"round", round, tms_cround},
    {"sign", tms_sign, tms_csign}, {"arg", tms_carg_d, carg_z},  {"sqrt", sqrt, csqrt},
    {"cbrt", cbrt, tms_ccbrt},     {"cos", tms_cos, tms_ccos},   {"sin", tms_sin, tms_csin},
    {"tan", tms_tan, tms_ctan},    {"acos", acos, cacos},        {"asin", asin, casin},
    {"atan", atan, catan},         {"cosh", cosh, ccosh},        {"sinh", sinh, csinh},
    {"tanh", tanh, ctanh},         {"acosh", acosh, cacosh},     {"asinh", asinh, casinh},
    {"atanh", atanh, catanh},      {"ln", log, tms_cln},         {"log2", log2, tms_clog2},
    {"log10", log10, tms_clog10}};

// Extended functions, may take more than one argument (stored in a comma separated string)
const tms_extf tms_g_extf[] = {{"avg", tms_avg},
                               {"min", tms_min},
                               {"max", tms_max},
                               {"integrate", tms_integrate},
                               {"derivative", tms_derivative},
                               {"logn", tms_logn},
                               {"hex", tms_hex},
                               {"oct", tms_oct},
                               {"bin", tms_bin},
                               {"rand", tms_rand},
                               {"int", tms_int}};

const tms_int_func tms_g_int_func[] = {{"not", tms_not},
                                       {"mask", tms_mask},
                                       {"mask_bit", tms_mask_bit},
                                       {"inv_mask", tms_inv_mask},
                                       {"ipv4_prefix", tms_ipv4_prefix},
                                       {"zeros", tms_zeros},
                                       {"ones", tms_ones},
                                       {"abs", tms_int_abs},
                                       {"parity", tms_parity}};

const tms_int_extf tms_g_int_extf[] = {{"rand", tms_int_rand},   {"rr", tms_rr},
                                       {"rl", tms_rl},           {"sr", tms_sr},
                                       {"sra", tms_sra},         {"sl", tms_sl},
                                       {"nand", tms_nand},       {"and", tms_and},
                                       {"xor", tms_xor},         {"nor", tms_nor},
                                       {"or", tms_or},           {"ipv4", tms_ipv4},
                                       {"dotted", tms_dotted},   {"mask_range", tms_mask_range},
                                       {"min", tms_int_min},     {"max", tms_int_max},
                                       {"float", tms_from_float}};

bool _tms_do_init = true;
bool _tms_debug = false;

pthread_mutex_t _parser_lock, _int_parser_lock;
pthread_mutex_t _ufunc_lock, _int_ufunc_lock;
pthread_mutex_t _variables_lock, _int_variables_lock;
pthread_mutex_t _evaluator_lock, _int_evaluator_lock;

char *tms_g_illegal_names[] = {"ans"};
const int tms_g_illegal_names_count = array_length(tms_g_illegal_names);

tms_var tms_g_builtin_vars[] = {{"i", I, true}, {"pi", M_PI, true}, {"e", M_E, true}, {"c", 299792458, true}};
hashmap *var_hmap, *int_var_hmap, *ufunc_hmap, *int_ufunc_hmap;
hashmap *rc_func_hmap, *extf_hmap, *int_func_hmap, *int_extf_hmap;

uint64_t tms_int_mask = 0xFFFFFFFF;

int8_t tms_int_mask_size = 32;

// Too many boilerplates, incoming!
// What they do is hash, compare and retrieve for all hashmaps
int _tms_var_compare(const void *a, const void *b, void *udata)
{
    const tms_var *va = a, *vb = b;
    return strcmp(va->name, vb->name);
}

uint64_t _tms_var_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const tms_var *v = item;
    return hashmap_xxhash3(v->name, strlen(v->name), seed0, seed1);
}

int _tms_int_var_compare(const void *a, const void *b, void *udata)
{
    const tms_int_var *va = a, *vb = b;
    return strcmp(va->name, vb->name);
}

uint64_t _tms_int_var_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const tms_int_var *v = item;
    return hashmap_xxhash3(v->name, strlen(v->name), seed0, seed1);
}

int _tms_ufunc_compare(const void *a, const void *b, void *udata)
{
    const tms_ufunc *va = a, *vb = b;
    return strcmp(va->name, vb->name);
}

uint64_t _tms_ufunc_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const tms_ufunc *v = item;
    return hashmap_xxhash3(v->name, strlen(v->name), seed0, seed1);
}

int _tms_int_ufunc_compare(const void *a, const void *b, void *udata)
{
    const tms_int_ufunc *va = a, *vb = b;
    return strcmp(va->name, vb->name);
}

uint64_t _tms_int_ufunc_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const tms_int_ufunc *v = item;
    return hashmap_xxhash3(v->name, strlen(v->name), seed0, seed1);
}

int _tms_rc_func_compare(const void *a, const void *b, void *udata)
{
    const tms_rc_func *va = a, *vb = b;
    return strcmp(va->name, vb->name);
}

uint64_t _tms_rc_func_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const tms_rc_func *v = item;
    return hashmap_xxhash3(v->name, strlen(v->name), seed0, seed1);
}

int _tms_extf_compare(const void *a, const void *b, void *udata)
{
    const tms_extf *va = a, *vb = b;
    return strcmp(va->name, vb->name);
}

uint64_t _tms_extf_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const tms_extf *v = item;
    return hashmap_xxhash3(v->name, strlen(v->name), seed0, seed1);
}

int _tms_int_func_compare(const void *a, const void *b, void *udata)
{
    const tms_int_func *va = a, *vb = b;
    return strcmp(va->name, vb->name);
}

uint64_t _tms_int_func_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const tms_int_func *v = item;
    return hashmap_xxhash3(v->name, strlen(v->name), seed0, seed1);
}

int _tms_int_extf_compare(const void *a, const void *b, void *udata)
{
    const tms_int_extf *va = a, *vb = b;
    return strcmp(va->name, vb->name);
}

uint64_t _tms_int_extf_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const tms_int_extf *v = item;
    return hashmap_xxhash3(v->name, strlen(v->name), seed0, seed1);
}

void _tms_free_rcfunc(void *item)
{
    tms_rc_func *a = item;
    free(a->name);
}

void _tms_free_extf(void *item)
{
    tms_extf *a = item;
    free(a->name);
}

void _tms_free_ufunc(void *item)
{
    tms_ufunc *a = item;
    free(a->name);
    tms_delete_math_expr(a->F);
}

void _tms_free_var(void *item)
{
    tms_var *a = item;
    free(a->name);
}

void _tms_free_int_func(void *item)
{
    tms_int_func *a = item;
    free(a->name);
}

void _tms_free_int_extf(void *item)
{
    tms_int_extf *a = item;
    free(a->name);
}

void _tms_free_int_ufunc(void *item)
{
    tms_int_ufunc *a = item;
    free(a->name);
    tms_delete_int_expr(a->F);
}

void _tms_free_int_var(void *item)
{
    tms_int_var *a = item;
    free(a->name);
}

const tms_var *tms_get_var_by_name(const char *name)
{
    tms_var tmp = {.name = name};
    return hashmap_get(var_hmap, &tmp);
}

const tms_int_var *tms_get_int_var_by_name(const char *name)
{
    tms_int_var tmp = {.name = name};
    return hashmap_get(int_var_hmap, &tmp);
}

const tms_rc_func *tms_get_rc_func_by_name(const char *name)
{
    tms_rc_func tmp = {.name = name};
    return hashmap_get(rc_func_hmap, &tmp);
}

const tms_extf *tms_get_extf_by_name(const char *name)
{
    tms_extf tmp = {.name = name};
    return hashmap_get(extf_hmap, &tmp);
}

const tms_int_func *tms_get_int_func_by_name(const char *name)
{
    tms_int_func tmp = {.name = name};
    return hashmap_get(int_func_hmap, &tmp);
}

const tms_int_extf *tms_get_int_extf_by_name(const char *name)
{
    tms_int_extf tmp = {.name = name};
    return hashmap_get(int_extf_hmap, &tmp);
}

const tms_ufunc *tms_get_ufunc_by_name(const char *name)
{
    tms_ufunc tmp = {.name = name};
    return hashmap_get(ufunc_hmap, &tmp);
}

const tms_int_ufunc *tms_get_int_ufunc_by_name(const char *name)
{
    tms_int_ufunc tmp = {.name = name};
    return hashmap_get(int_ufunc_hmap, &tmp);
}

tms_var *tms_get_all_vars(size_t *count, bool sort)
{
    return hashmap_to_array(var_hmap, count, sort);
}

tms_int_var *tms_get_all_int_vars(size_t *count, bool sort)
{
    return hashmap_to_array(int_var_hmap, count, sort);
}

tms_rc_func *tms_get_all_rc_func(size_t *count, bool sort)
{
    return hashmap_to_array(rc_func_hmap, count, sort);
}

tms_extf *tms_get_all_extf(size_t *count, bool sort)
{
    return hashmap_to_array(extf_hmap, count, sort);
}

tms_ufunc *tms_get_all_ufunc(size_t *count, bool sort)
{
    return hashmap_to_array(ufunc_hmap, count, sort);
}

tms_int_func *tms_get_all_int_func(size_t *count, bool sort)
{
    return hashmap_to_array(int_func_hmap, count, sort);
}

tms_int_extf *tms_get_all_int_extf(size_t *count, bool sort)
{
    return hashmap_to_array(int_extf_hmap, count, sort);
}

tms_int_ufunc *tms_get_all_int_ufunc(size_t *count, bool sort)
{
    return hashmap_to_array(int_ufunc_hmap, count, sort);
}

bool tms_function_exists(const char *name)
{
    if (tms_get_rc_func_by_name(name) != NULL || tms_get_extf_by_name(name) != NULL ||
        tms_get_ufunc_by_name(name) != NULL)
        return true;
    else
        return false;
}

bool tms_int_function_exists(const char *name)
{
    if (tms_get_int_func_by_name(name) != NULL || tms_get_int_extf_by_name(name) != NULL ||
        tms_get_int_ufunc_by_name(name) != NULL)
        return true;
    else
        return false;
}

bool tms_builtin_function_exists(const char *name)
{
    if (tms_get_rc_func_by_name(name) != NULL || tms_get_extf_by_name(name) != NULL)
        return true;
    else
        return false;
}

bool tms_builtin_int_function_exists(const char *name)
{
    if (tms_get_int_func_by_name(name) != NULL || tms_get_int_extf_by_name(name) != NULL)
        return true;
    else
        return false;
}

int tms_remove_var(const char *name)
{
    const tms_var t = {.name = name}, *check;
    check = hashmap_get(var_hmap, &t);
    if (check == NULL)
        return -1;
    // Can't remove a built in variable, so return 1 to tell it
    if (check->is_constant)
        return 1;
    else
        return hashmap_delete_and_free(var_hmap, &t);
}

int tms_remove_int_var(const char *name)
{
    const tms_int_var t = {.name = name}, *check;
    check = hashmap_get(int_var_hmap, &t);
    if (check == NULL)
        return -1;
    // Can't remove a built in variable, so return 1 to tell it
    if (check->is_constant)
        return 1;
    else
        return hashmap_delete_and_free(int_var_hmap, &t);
}

int tms_remove_ufunc(const char *name)
{
    tms_ufunc t = {.name = name};
    return hashmap_delete_and_free(ufunc_hmap, &t);
}

int tms_remove_int_ufunc(const char *name)
{
    tms_int_ufunc t = {.name = name};
    return hashmap_delete_and_free(int_ufunc_hmap, &t);
}

void tmsolve_init()
{
    if (_tms_do_init)
    {
        // Initialize mutexes
        pthread_mutex_init(&_parser_lock, NULL);
        pthread_mutex_init(&_int_parser_lock, NULL);
        pthread_mutex_init(&_evaluator_lock, NULL);
        pthread_mutex_init(&_int_evaluator_lock, NULL);
        pthread_mutex_init(&_ufunc_lock, NULL);
        pthread_mutex_init(&_int_ufunc_lock, NULL);
        pthread_mutex_init(&_variables_lock, NULL);
        pthread_mutex_init(&_int_variables_lock, NULL);

        // Seed the random number generator
        srand(time(NULL));

        // Prepare hashmaps
        var_hmap =
            hashmap_new(sizeof(tms_var), 0, rand(), rand(), _tms_var_hash, _tms_var_compare, _tms_free_var, NULL);

        int_var_hmap = hashmap_new(sizeof(tms_int_var), 0, rand(), rand(), _tms_int_var_hash, _tms_int_var_compare,
                                   _tms_free_int_var, NULL);

        ufunc_hmap = hashmap_new(sizeof(tms_ufunc), 0, rand(), rand(), _tms_ufunc_hash, _tms_ufunc_compare,
                                 _tms_free_ufunc, NULL);

        int_ufunc_hmap = hashmap_new(sizeof(tms_int_ufunc), 0, rand(), rand(), _tms_int_ufunc_hash,
                                     _tms_int_ufunc_compare, _tms_free_int_ufunc, NULL);

        rc_func_hmap = hashmap_new(sizeof(tms_rc_func), 0, rand(), rand(), _tms_rc_func_hash, _tms_rc_func_compare,
                                   _tms_free_rcfunc, NULL);

        extf_hmap =
            hashmap_new(sizeof(tms_extf), 0, rand(), rand(), _tms_extf_hash, _tms_extf_compare, _tms_free_extf, NULL);

        int_func_hmap = hashmap_new(sizeof(tms_int_func), 0, rand(), rand(), _tms_int_func_hash, _tms_int_func_compare,
                                    _tms_free_int_func, NULL);

        int_extf_hmap = hashmap_new(sizeof(tms_int_extf), 0, rand(), rand(), _tms_int_extf_hash, _tms_int_extf_compare,
                                    _tms_free_int_extf, NULL);
        int i;
        for (i = 0; i < array_length(tms_g_builtin_vars); ++i)
            hashmap_set(var_hmap, tms_g_builtin_vars + i);

        for (i = 0; i < array_length(tms_g_rc_func); ++i)
            hashmap_set(rc_func_hmap, tms_g_rc_func + i);

        for (i = 0; i < array_length(tms_g_extf); ++i)
            hashmap_set(extf_hmap, tms_g_extf + i);

        for (i = 0; i < array_length(tms_g_int_func); ++i)
            hashmap_set(int_func_hmap, tms_g_int_func + i);

        for (i = 0; i < array_length(tms_g_int_extf); ++i)
            hashmap_set(int_extf_hmap, tms_g_int_extf + i);

        _tms_do_init = false;
    }
}

void tms_lock_parser(int variant)
{
    switch (variant)
    {
    case TMS_PARSER:
        pthread_mutex_lock(&_parser_lock);
        pthread_mutex_lock(&_variables_lock);
        pthread_mutex_lock(&_ufunc_lock);
        return;

    case TMS_INT_PARSER:
        pthread_mutex_lock(&_int_parser_lock);
        pthread_mutex_lock(&_int_variables_lock);
        pthread_mutex_lock(&_int_ufunc_lock);
        return;

    default:
        fputs("libtmsolve: Error while locking parsers: invalid ID...", stderr);
        abort();
    }
}

void tms_unlock_parser(int variant)
{
    switch (variant)
    {
    case TMS_PARSER:
        pthread_mutex_unlock(&_parser_lock);
        pthread_mutex_unlock(&_variables_lock);
        pthread_mutex_unlock(&_ufunc_lock);
        return;

    case TMS_INT_PARSER:
        pthread_mutex_unlock(&_int_parser_lock);
        pthread_mutex_unlock(&_int_variables_lock);
        pthread_mutex_unlock(&_int_ufunc_lock);
        return;

    default:
        fputs("libtmsolve: Error while unlocking parsers: invalid ID...", stderr);
        abort();
    }
}

void tms_lock_evaluator(int variant)
{
    switch (variant)
    {
    case TMS_EVALUATOR:
        pthread_mutex_lock(&_evaluator_lock);
        pthread_mutex_lock(&_ufunc_lock);
        return;

    case TMS_INT_EVALUATOR:
        pthread_mutex_lock(&_int_evaluator_lock);
        pthread_mutex_lock(&_int_ufunc_lock);
        return;

    default:
        fputs("libtmsolve: Error while locking evaluator: invalid ID...", stderr);
        abort();
    }
}

void tms_unlock_evaluator(int variant)
{
    switch (variant)
    {
    case TMS_EVALUATOR:
        pthread_mutex_unlock(&_evaluator_lock);
        pthread_mutex_unlock(&_ufunc_lock);
        return;

    case TMS_INT_EVALUATOR:
        pthread_mutex_unlock(&_int_evaluator_lock);
        pthread_mutex_unlock(&_int_ufunc_lock);
        return;

    default:
        fputs("libtmsolve: Error while unlocking evaluator: invalid ID...", stderr);
        abort();
    }
}

void tms_lock_vars(int variant)
{
    switch (variant)
    {
    case TMS_V_DOUBLE:
        pthread_mutex_lock(&_variables_lock);
        return;
    case TMS_V_INT64:
        pthread_mutex_lock(&_int_variables_lock);
        return;

    default:
        fputs("libtmsolve: Error while locking variables: invalid ID...", stderr);
        abort();
    }
}

void tms_unlock_vars(int variant)
{
    switch (variant)
    {
    case TMS_V_DOUBLE:
        pthread_mutex_unlock(&_variables_lock);
        return;
    case TMS_V_INT64:
        pthread_mutex_unlock(&_int_variables_lock);
        return;

    default:
        fputs("libtmsolve: Error while unlocking variables: invalid ID...", stderr);
        abort();
    }
}

void tms_lock_ufuncs(int variant)
{
    switch (variant)
    {
    case TMS_V_DOUBLE:
        pthread_mutex_lock(&_ufunc_lock);
        return;
    case TMS_V_INT64:
        pthread_mutex_lock(&_int_ufunc_lock);
        return;

    default:
        fputs("libtmsolve: Error while locking user functions: invalid ID...", stderr);
        abort();
    }
}

void tms_unlock_ufuncs(int variant)
{
    switch (variant)
    {
    case TMS_V_DOUBLE:
        pthread_mutex_unlock(&_ufunc_lock);
        return;
    case TMS_V_INT64:
        pthread_mutex_unlock(&_int_ufunc_lock);
        return;

    default:
        fputs("libtmsolve: Error while unlocking user functions: invalid ID...", stderr);
        abort();
    }
}

void tmsolve_reset()
{
    size_t len;
    tms_lock_vars(TMS_V_DOUBLE);
    tms_var *all_vars = tms_get_all_vars(&len, false);
    // Delete all user variables (not the constants set during initialization)
    if (all_vars != NULL)
        for (size_t i = 0; i < len; ++i)
            if (!all_vars[i].is_constant)
                hashmap_delete(var_hmap, all_vars + i);
    tms_g_ans = 0;
    free(all_vars);
    tms_unlock_vars(TMS_V_DOUBLE);

    tms_set_int_mask(32);
    tms_lock_vars(TMS_V_INT64);
    tms_int_var *all_int_vars = tms_get_all_int_vars(&len, false);
    // Same as above
    if (all_int_vars != NULL)
        for (size_t i = 0; i < len; ++i)
            if (!all_int_vars[i].is_constant)
                hashmap_delete(int_var_hmap, all_int_vars + i);
    tms_g_int_ans = 0;
    free(all_int_vars);
    tms_unlock_vars(TMS_V_INT64);

    tms_lock_ufuncs(TMS_V_DOUBLE);
    hashmap_clear(ufunc_hmap, true);
    tms_unlock_ufuncs(TMS_V_DOUBLE);

    tms_lock_ufuncs(TMS_V_INT64);
    hashmap_clear(int_ufunc_hmap, true);
    tms_unlock_ufuncs(TMS_V_INT64);
}

int tms_set_int_mask(int size_in_bits)
{
    if (size_in_bits < 0 || size_in_bits > 64)
        return 1;

    // Use the fact that a value of (2^n)-1 doesn't have any intersection with 2^n in binary
    // So when AND ing the answer will be zero
    if (!((size_in_bits != 0) && ((size_in_bits & (size_in_bits - 1)) == 0)))
        return 2;

    // Lock the evaluator (implicitly locks the parser due to common dependency on ufunc lock)
    tms_lock_evaluator(TMS_INT_EVALUATOR);
    if (size_in_bits == 64)
    {
        tms_int_mask = ~(int64_t)0;
        tms_int_mask_size = 64;
    }
    else
    {
        tms_int_mask = ((int64_t)1 << size_in_bits) - 1;
        tms_int_mask_size = size_in_bits;
    }
    tms_unlock_evaluator(TMS_INT_EVALUATOR);
    return 0;
}

bool _tms_validate_args_count(int expected, int actual, int facility_id)
{
    if (expected == actual)
        return true;
    else if (expected > actual)
        tms_save_error(facility_id, TOO_FEW_ARGS, EH_FATAL, NULL, 0);
    else if (expected < actual)
        tms_save_error(facility_id, TOO_MANY_ARGS, EH_FATAL, NULL, 0);
    return false;
}

bool _tms_validate_args_count_range(int actual, int min, int max, int facility_id)
{
    // If max is set to -1 this means no argument limit (for functions like min and max)
    if (max != -1 && actual > max)
        tms_save_error(facility_id, TOO_MANY_ARGS, EH_FATAL, NULL, 0);
    else if (actual < min)
        tms_save_error(facility_id, TOO_FEW_ARGS, EH_FATAL, NULL, 0);
    else
        return true;

    return false;
}

int _tms_set_var_unsafe(const char *name, double complex value, bool is_constant)
{
    if (isnan(creal(value)) || isnan(cimag(value)))
        return -1;

    // Check if the name has illegal characters
    if (tms_valid_name(name) == false)
    {
        tms_save_error(TMS_PARSER, INVALID_NAME, EH_FATAL, NULL, 0);
        return -1;
    }

    // Check if the name is allowed
    if (!tms_legal_name(name))
    {
        tms_save_error(TMS_PARSER, ILLEGAL_NAME, EH_FATAL, NULL, 0);
        return -1;
    }

    if (tms_function_exists(name))
    {
        tms_save_error(TMS_PARSER, VAR_NAME_MATCHES_FUNCTION, EH_FATAL, NULL, 0);
        return -1;
    }

    const tms_var *existing_var = tms_get_var_by_name(name);
    char *tmp_name;
    // Var already exists
    if (existing_var != NULL)
    {
        if (existing_var->is_constant)
        {
            tms_save_error(TMS_PARSER, OVERWRITE_CONST_VARIABLE, EH_FATAL, NULL, 0);
            return -1;
        }
        // Reuse the already allocated name
        tmp_name = existing_var->name;
    }
    else
        tmp_name = strdup(name);

    tms_var v = {.name = tmp_name, .value = value, .is_constant = is_constant};
    hashmap_set(var_hmap, &v);
    return 0;
}

int tms_set_var(const char *name, double complex value, bool is_constant)
{
    pthread_mutex_lock(&_variables_lock);
    int status = _tms_set_var_unsafe(name, value, is_constant);
    pthread_mutex_unlock(&_variables_lock);
    return status;
}

int _tms_set_int_var_unsafe(const char *name, int64_t value, bool is_constant)
{
    value = tms_sign_extend(value);
    // Check if the name has illegal characters
    if (tms_valid_name(name) == false)
    {
        tms_save_error(TMS_INT_PARSER, INVALID_NAME, EH_FATAL, NULL, 0);
        return -1;
    }

    // Check if the name is allowed
    if (!tms_legal_name(name))
    {
        tms_save_error(TMS_INT_PARSER, ILLEGAL_NAME, EH_FATAL, NULL, 0);
        return -1;
    }

    if (tms_int_function_exists(name))
    {
        tms_save_error(TMS_INT_PARSER, VAR_NAME_MATCHES_FUNCTION, EH_FATAL, NULL, 0);
        return -1;
    }

    const tms_int_var *existing_var = tms_get_int_var_by_name(name);
    char *tmp_name;
    // Var already exists
    if (existing_var != NULL)
    {
        tmp_name = existing_var->name;
        if (existing_var->is_constant)
        {
            tms_save_error(TMS_INT_PARSER, OVERWRITE_CONST_VARIABLE, EH_FATAL, NULL, 0);
            return -1;
        }
    }
    else
        tmp_name = strdup(name);

    tms_int_var v = {.name = tmp_name, .value = value, .is_constant = is_constant};
    hashmap_set(int_var_hmap, &v);
    return 0;
}

int tms_set_int_var(const char *name, int64_t value, bool is_constant)
{
    pthread_mutex_lock(&_int_variables_lock);
    int status = _tms_set_int_var_unsafe(name, value, is_constant);
    pthread_mutex_unlock(&_int_variables_lock);
    return status;
}

// Get all user defined functions
hashset *get_all_ufunc_references(const char *fname)
{
    const tms_ufunc *F = tms_get_ufunc_by_name(fname);
    if (F == NULL)
        return NULL;

    hashset *all_refs = hashset_new();
    // If hashset init failed, we have some serious issues
    if (all_refs == NULL)
    {
        fputs(INTERNAL_ERROR "\n", stderr);
        abort();
    }

    tms_math_expr *M = F->F;
    tms_math_subexpr *S = M->S;

    // Get a set of all user functions referenced
    for (int i = 0; i < M->subexpr_count; ++i)
        if (S[i].func_type == TMS_F_USER)
            hashset_set(all_refs, S[i].func.user);

    if (hashset_count(all_refs) == 0)
    {
        hashset_free(all_refs);
        return NULL;
    }
    else
        return all_refs;
}

bool _ufunc_is_within_arglist(const char *referrer, const char *fname_wparenthesis)
{
    const tms_ufunc *tmp = tms_get_ufunc_by_name(referrer);
    tms_math_expr *M = tmp->F;
    // For every subexpression, check if it is a user or extended function
    // If so, search the argument strings for any reference of the target function
    for (int i = 0; i < M->subexpr_count; ++i)
    {
        if (M->S[i].f_args != NULL)
        {
            tms_arg_list *L = M->S[i].f_args;
            for (int j = 0; j < L->count; ++j)
                if (tms_f_search(L->arguments[j], fname_wparenthesis, 0, false) != -1)
                    return true;
        }
    }
    return false;
}

bool is_ufunc_referenced_by(const char *referrer, const char *target)
{
    char target_wparenthesis[strlen(target) + 2];
    // Add a ( to the function name so forward search would find an actual reference
    strcpy(target_wparenthesis, target);
    strcat(target_wparenthesis, "(");
    hashset *all_refs = get_all_ufunc_references(referrer);

    if (strcmp(referrer, target) == 0)
    {
        if (all_refs != NULL && hashset_get(all_refs, target) != NULL)
        {
            hashset_free(all_refs);
            return true;
        }
        else
        // Self reference within my subexpressions arglist
        {
            hashset_free(all_refs);
            return _ufunc_is_within_arglist(target, target_wparenthesis);
        }
    }

    if (all_refs == NULL)
        return false;

    // Direct reference
    if (hashset_get(all_refs, target) != NULL)
    {
        hashset_free(all_refs);
        return true;
    }

    // Indirect reference
    size_t len;
    char **refs_names = hashset_to_array(all_refs, &len, false);

    hashset_free(all_refs);
    const tms_ufunc *tmp;

    // Iterating over all user functions referred to by the referrer
    for (size_t i = 0; i < len; ++i)
    {
        tmp = tms_get_ufunc_by_name(refs_names[i]);
        // The function is not found, skip
        if (tmp == NULL)
            continue;

        else if (_ufunc_is_within_arglist(refs_names[i], target_wparenthesis))
        {
            free(refs_names);
            return true;
        }

        // Recursive search in referred functions
        if (is_ufunc_referenced_by(refs_names[i], target))
        {
            free(refs_names);
            return true;
        }
    }
    free(refs_names);
    return false;
}

bool _tms_ufunc_has_bad_refs(const char *fname)
{
    // Self reference check
    if (is_ufunc_referenced_by(fname, fname))
    {
        tms_save_error(TMS_PARSER, NO_FSELF_REFERENCE, EH_FATAL, NULL, 0);
        return true;
    }

    hashset *all_refs = get_all_ufunc_references(fname);

    if (all_refs == NULL)
        return false;

    // Now check if this is referenced elsewhere
    size_t len;
    char **refs_names = hashset_to_array(all_refs, &len, false);
    if (refs_names == NULL)
    {
        fputs(INTERNAL_ERROR "\n", stderr);
        abort();
    }

    // Check for every referenced function if it refers to this function
    for (size_t i = 0; i < len; ++i)
    {
        if (is_ufunc_referenced_by(refs_names[i], fname))
        {
            tms_save_error(TMS_PARSER, NO_FCIRCULAR_REFERENCE, EH_FATAL, NULL, 0);
            free(refs_names);
            hashset_free(all_refs);
            return true;
        }
    }
    free(refs_names);
    hashset_free(all_refs);
    return false;
}

int tms_set_ufunction(const char *fname, const char *function_args, const char *function)
{
    const tms_ufunc *old = tms_get_ufunc_by_name(fname);

    // Skip name related verification if it already exists
    if (old == NULL)
    {
        // Check if the name has illegal characters
        if (tms_valid_name(fname) == false)
        {
            tms_save_error(TMS_PARSER, INVALID_NAME, EH_FATAL, function, 0);
            return -1;
        }

        // Check if the function name is allowed
        if (!tms_legal_name(fname))
        {
            tms_save_error(TMS_PARSER, ILLEGAL_NAME, EH_FATAL, function, 0);
            return -1;
        }

        // Check if the name was already used by builtin functions
        if (tms_builtin_function_exists(fname))
        {
            tms_save_error(TMS_PARSER, NO_FUNCTION_SHADOWING, EH_FATAL, function, 0);
            return -1;
        }

        // Check if the name is used by a variable
        if (tms_get_var_by_name(fname) != NULL)
        {
            tms_save_error(TMS_PARSER, FUNCTION_NAME_MATCHES_VAR, EH_FATAL, function, 0);
            return -1;
        }
    }

    // The received args are in a comma separated string, we convert them to an argument list
    tms_arg_list *arg_list = tms_get_args(function_args);

    if (arg_list->count > 64)
    {
        tms_save_error(TMS_PARSER, TOO_MANY_LABELS, EH_FATAL, function, 0);
        tms_free_arg_list(arg_list);
        return -1;
    }

    // Check that names are unique
    if (!tms_is_unique_string_array(arg_list->arguments, arg_list->count))
    {
        tms_save_error(TMS_PARSER, LABELS_NOT_UNIQUE, EH_FATAL, function, 0);
        tms_free_arg_list(arg_list);
        return -1;
    }

    // Check that argument names are valid
    for (int j = 0; j < arg_list->count; ++j)
    {
        if (!tms_valid_name(arg_list->arguments[j]))
        {
            // Find the index of this argument in the argument list
            tms_save_error(TMS_PARSER, INVALID_NAME, EH_FATAL, function_args,
                           tms_f_search(function_args, arg_list->arguments[j], 0, true));
            tms_free_arg_list(arg_list);
            return -1;
        }
    }
    tms_math_expr *new = tms_parse_expr(function, ENABLE_CMPLX, arg_list);

    if (new == NULL)
        return -1;

    // Function already exists
    if (old != NULL)
    {
        // The old ufunc members is kept to either free the old function or restore it on failure of the new function
        tms_ufunc old_F;
        old_F = *old;

        tms_ufunc tmp = {.F = new, .name = old->name};

        // We need to update the hashmap because the function checks will lookup the name in the hashmap
        // otherwise we will get the old function checked instead
        hashmap_set(ufunc_hmap, &tmp);
        if (_tms_ufunc_has_bad_refs(fname))
        {
            // Restore the original function since the new one is problematic
            hashmap_set(ufunc_hmap, &old_F);
            tms_delete_math_expr(tmp.F);
            return -1;
        }
        else
        {
            tms_delete_math_expr(old_F.F);
            return 0;
        }
    }
    else
    {
        tms_ufunc tmp = {.F = new, .name = strdup(fname)};
        hashmap_set(ufunc_hmap, &tmp);
        return 0;
    }
    return -1;
}

// Get all user defined functions
hashset *get_all_int_ufunc_references(const char *fname)
{
    const tms_int_ufunc *F = tms_get_int_ufunc_by_name(fname);
    if (F == NULL)
        return NULL;

    hashset *all_refs = hashset_new();
    // If hashset init failed, we have some serious issues
    if (all_refs == NULL)
    {
        fputs(INTERNAL_ERROR "\n", stderr);
        abort();
    }

    tms_int_expr *M = F->F;
    tms_int_subexpr *S = M->S;

    // Get a set of all user functions referenced
    for (int i = 0; i < M->subexpr_count; ++i)
        if (S[i].func_type == TMS_F_INT_USER)
            hashset_set(all_refs, S[i].func.user);

    if (hashset_count(all_refs) == 0)
    {
        hashset_free(all_refs);
        return NULL;
    }
    else
        return all_refs;
}

bool _int_ufunc_is_within_arglist(const char *referrer, const char *fname_wparenthesis)
{
    const tms_int_ufunc *tmp = tms_get_int_ufunc_by_name(referrer);
    tms_int_expr *M = tmp->F;
    // For every subexpression, check if it is a user or extended function
    // If so, search the argument strings for any reference of the target function
    for (int i = 0; i < M->subexpr_count; ++i)
    {
        if (M->S[i].f_args != NULL)
        {
            tms_arg_list *L = M->S[i].f_args;
            for (int j = 0; j < L->count; ++j)
                if (tms_f_search(L->arguments[j], fname_wparenthesis, 0, false) != -1)
                    return true;
        }
    }
    return false;
}

bool is_int_ufunc_referenced_by(const char *referrer, const char *target)
{
    char target_wparenthesis[strlen(target) + 2];
    // Add a ( to the function name so forward search would find an actual reference
    strcpy(target_wparenthesis, target);
    strcat(target_wparenthesis, "(");
    hashset *all_refs = get_all_int_ufunc_references(referrer);

    if (strcmp(referrer, target) == 0)
    {
        if (all_refs != NULL && hashset_get(all_refs, target) != NULL)
        {
            hashset_free(all_refs);
            return true;
        }
        else
        // Self reference within my subexpressions arglist
        {
            hashset_free(all_refs);
            return _int_ufunc_is_within_arglist(target, target_wparenthesis);
        }
    }

    if (all_refs == NULL)
        return false;

    // Direct reference
    if (hashset_get(all_refs, target) != NULL)
    {
        hashset_free(all_refs);
        return true;
    }

    // Indirect reference
    size_t len;
    char **refs_names = hashset_to_array(all_refs, &len, false);

    hashset_free(all_refs);
    const tms_int_ufunc *tmp;

    // Iterating over all user functions referred to by the referrer
    for (size_t i = 0; i < len; ++i)
    {
        tmp = tms_get_int_ufunc_by_name(refs_names[i]);
        // The function is not found, skip
        if (tmp == NULL)
            continue;

        else if (_int_ufunc_is_within_arglist(refs_names[i], target_wparenthesis))
        {
            free(refs_names);
            return true;
        }

        // Recursive search in referred functions
        if (is_int_ufunc_referenced_by(refs_names[i], target))
        {
            free(refs_names);
            return true;
        }
    }
    free(refs_names);
    return false;
}

bool _tms_int_ufunc_has_bad_refs(const char *fname)
{
    // Self reference check
    if (is_int_ufunc_referenced_by(fname, fname))
    {
        tms_save_error(TMS_INT_PARSER, NO_FSELF_REFERENCE, EH_FATAL, NULL, 0);
        return true;
    }

    hashset *all_refs = get_all_int_ufunc_references(fname);

    if (all_refs == NULL)
        return false;

    // Now check if this is referenced elsewhere
    size_t len;
    char **refs_names = hashset_to_array(all_refs, &len, false);
    if (refs_names == NULL)
    {
        fputs(INTERNAL_ERROR "\n", stderr);
        abort();
    }

    // Check for every referenced function if it refers to this function
    for (size_t i = 0; i < len; ++i)
    {
        if (is_int_ufunc_referenced_by(refs_names[i], fname))
        {
            tms_save_error(TMS_INT_PARSER, NO_FCIRCULAR_REFERENCE, EH_FATAL, NULL, 0);
            free(refs_names);
            hashset_free(all_refs);
            return true;
        }
    }
    free(refs_names);
    hashset_free(all_refs);
    return false;
}

int tms_set_int_ufunction(const char *fname, const char *function_args, const char *function)
{
    const tms_int_ufunc *old = tms_get_int_ufunc_by_name(fname);

    // Skip name related verification if it already exists
    if (old == NULL)
    {
        // Check if the name has illegal characters
        if (tms_valid_name(fname) == false)
        {
            tms_save_error(TMS_INT_PARSER, INVALID_NAME, EH_FATAL, function, 0);
            return -1;
        }

        // Check if the function name is allowed
        if (!tms_legal_name(fname))
        {
            tms_save_error(TMS_INT_PARSER, ILLEGAL_NAME, EH_FATAL, function, 0);
            return -1;
        }

        // Check if the name was already used by builtin functions
        if (tms_builtin_int_function_exists(fname))
        {
            tms_save_error(TMS_INT_PARSER, NO_FUNCTION_SHADOWING, EH_FATAL, function, 0);
            return -1;
        }

        // Check if the name is used by a variable
        if (tms_get_int_var_by_name(fname) != NULL)
        {
            tms_save_error(TMS_INT_PARSER, FUNCTION_NAME_MATCHES_VAR, EH_FATAL, function, 0);
            return -1;
        }
    }

    // The received args are in a comma separated string, we convert them to an argument list
    tms_arg_list *arg_list = tms_get_args(function_args);

    if (arg_list->count > 64)
    {
        tms_save_error(TMS_INT_PARSER, TOO_MANY_LABELS, EH_FATAL, function, 0);
        tms_free_arg_list(arg_list);
        return -1;
    }

    // Check that names are unique
    if (!tms_is_unique_string_array(arg_list->arguments, arg_list->count))
    {
        tms_save_error(TMS_INT_PARSER, LABELS_NOT_UNIQUE, EH_FATAL, function, 0);
        tms_free_arg_list(arg_list);
        return -1;
    }

    // Check that argument names are valid
    for (int j = 0; j < arg_list->count; ++j)
    {
        if (!tms_valid_name(arg_list->arguments[j]))
        {
            tms_save_error(TMS_INT_PARSER, INVALID_NAME, EH_FATAL, function_args,
                           tms_f_search(function_args, arg_list->arguments[j], 0, true));
            tms_free_arg_list(arg_list);
            return -1;
        }
    }
    tms_int_expr *new = tms_parse_int_expr(function, 0, arg_list);

    if (new == NULL)
        return -1;

    // Function already exists
    if (old != NULL)
    {
        // The old ufunc members is kept to either free the old function or restore it on failure of the new function
        tms_int_ufunc old_F;
        old_F = *old;

        tms_int_ufunc tmp = {.F = new, .name = old->name};

        // We need to update the hashmap because the function checks will lookup the name in the hashmap
        // otherwise we will get the old function checked instead
        hashmap_set(int_ufunc_hmap, &tmp);
        if (_tms_int_ufunc_has_bad_refs(fname))
        {
            // Restore the original function since the new one is problematic
            hashmap_set(int_ufunc_hmap, &old_F);
            tms_delete_int_expr(tmp.F);
            return -1;
        }
        else
        {
            tms_delete_int_expr(old_F.F);
            return 0;
        }
    }
    else
    {
        tms_int_ufunc tmp = {.F = new, .name = strdup(fname)};
        hashmap_set(int_ufunc_hmap, &tmp);
        return 0;
    }
    return -1;
}
char **tms_smode_autocompletion_helper(const char *name)
{
    size_t max_count =
        array_length(tms_g_rc_func) + array_length(tms_g_extf) + hashmap_count(ufunc_hmap) + hashmap_count(var_hmap);
    size_t i, next = 0;
    // +1 for the extra NULL
    char **matches = malloc((max_count + 1) * sizeof(char *));

    // Real/Complex functions
    for (i = 0; i < array_length(tms_g_rc_func); ++i)
        if (_tms_string_is_prefix(tms_g_rc_func[i].name, name))
            matches[next++] = tms_strcat_dup(tms_g_rc_func[i].name, "(");

    // Extended
    for (i = 0; i < array_length(tms_g_extf); ++i)
        if (_tms_string_is_prefix(tms_g_extf[i].name, name))
            matches[next++] = tms_strcat_dup(tms_g_extf[i].name, "(");

    // User functions
    size_t count;
    tms_ufunc *ufuncs = hashmap_to_array(ufunc_hmap, &count, true);
    if (ufuncs != NULL)
        for (i = 0; i < count; ++i)
            if (_tms_string_is_prefix(ufuncs[i].name, name))
                matches[next++] = tms_strcat_dup(ufuncs[i].name, "(");

    // Variables
    tms_var *vars = hashmap_to_array(var_hmap, &count, true);
    if (vars != NULL)
        for (i = 0; i < count; ++i)
            if (_tms_string_is_prefix(vars[i].name, name))
                matches[next++] = strdup(vars[i].name);

    // Readline needs a NULL to know the end of the array
    matches[next++] = NULL;
    matches = realloc(matches, next * sizeof(char *));
    free(ufuncs);
    free(vars);
    return matches;
}

char **tms_imode_autocompletion_helper(const char *name)
{
    size_t max_count = array_length(tms_g_int_func) + array_length(tms_g_int_extf) + hashmap_count(int_ufunc_hmap) +
                       hashmap_count(int_var_hmap);
    size_t i, next = 0;
    // +1 for the extra NULL
    char **matches = malloc((max_count + 1) * sizeof(char *));

    // Int functions
    for (i = 0; i < array_length(tms_g_int_func); ++i)
        if (_tms_string_is_prefix(tms_g_int_func[i].name, name))
            matches[next++] = tms_strcat_dup(tms_g_int_func[i].name, "(");

    // Extended
    for (i = 0; i < array_length(tms_g_int_extf); ++i)
        if (_tms_string_is_prefix(tms_g_int_extf[i].name, name))
            matches[next++] = tms_strcat_dup(tms_g_int_extf[i].name, "(");

    // User functions
    size_t count;
    tms_ufunc *int_ufuncs = hashmap_to_array(int_ufunc_hmap, &count, true);
    if (int_ufuncs != NULL)
        for (i = 0; i < count; ++i)
            if (_tms_string_is_prefix(int_ufuncs[i].name, name))
                matches[next++] = tms_strcat_dup(int_ufuncs[i].name, "(");

    // Variables
    tms_var *int_vars = hashmap_to_array(int_var_hmap, &count, true);
    if (int_vars != NULL)
        for (i = 0; i < count; ++i)
            if (_tms_string_is_prefix(int_vars[i].name, name))
                matches[next++] = strdup(int_vars[i].name);

    // Readline needs a NULL to know the end of the array
    matches[next++] = NULL;
    matches = realloc(matches, next * sizeof(char *));
    free(int_ufuncs);
    free(int_vars);
    return matches;
}

int compare_ints(const void *a, const void *b)
{
    if (*(int *)a < *(int *)b)
        return -1;
    else if (*(int *)a > *(int *)b)
        return 1;
    else
        return 0;
}

int compare_ints_reverse(const void *a, const void *b)
{
    if (*(int *)a < *(int *)b)
        return 1;
    else if (*(int *)a > *(int *)b)
        return -1;
    else
        return 0;
}

double tms_random_weight()
{
    return (double)rand() / RAND_MAX;
}
