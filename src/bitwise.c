/*
Copyright (C) 2023 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/

#include "internals.h"
#include "string_tools.h"

int64_t tms_not(int64_t value)
{
    return ~value;
}

int64_t _tms_rotate_circular(tms_arg_list *args, char direction)
{
    if (_tms_validate_args_count(2, args->count) == false)
    {
        tms_error_bit = 1;
        return -1;
    }
    int64_t value, shift;
    value = tms_read_int_value(args->arguments[0], 0);
    if (tms_error_bit == 1)
    {
        return -1;
    }
    shift = tms_read_int_value(args->arguments[1], 0);
    if (tms_error_bit == 1)
    {
        return -1;
    }

    shift %= tms_int_mask_size;
    switch (direction)
    {
    case 'r':
        return (value >> shift | value << (tms_int_mask_size - shift)) & tms_int_mask;

    case 'l':
        return (value << shift | value >> (tms_int_mask_size - shift)) & tms_int_mask;

    default:
        tms_error_handler(EH_SAVE, INTERNAL_ERROR, EH_FATAL_ERROR, -1);
        tms_error_bit = 1;
        return -1;
    }
}

int64_t tms_rrc(tms_arg_list *args)
{
    return _tms_rotate_circular(args, 'r');
}

int64_t tms_rlc(tms_arg_list *args)
{
    return _tms_rotate_circular(args, 'l');
}
