# Changelog

## 2.1.0 - 2024-02-24

### Changes

- Add int functions `ipv4_prefix()`, `ones()`, `zeros()`.
- Modulo operator now has the same precedence as multiplication and division in int variants.
- `tms_set_int_mask()` now enforces masks of width a power of 2.
- Catch improper shift amount in int mode shift functions.
- Catch negative rotation amount in int mode `rr()` and `rl()`.

### Fixes

- Fix mask generation issue in `tms_set_int_mask()` for 64 bit width.
- Fix missing space while printing with width of 64 bits.

## 2.0.1 - 2024-02-21

### Fixes

- Fix broken behavior of `tms_print_bin()` for int masks of size different than 32.
- Fix error printer not prepending `...` when the error index is between 24 and 49.
- Fix the edge case where `+-` is treated as `+` (bug in `tms_combine_add_subtract()`).

### Added

- `tms_set_int_mask()` to set the int mask size while temporarily locking int parser and evaluator.

## 2.0.0 - 2024-02-20

This is a major, API breaking release.

### Changes

- Perform syntax checks within the parsers, not separately in `tms_syntax_check()`.
- Many changes to `tms_math_expr` and `tms_int_expr`.
- Parsers no longer assume that the string received as argument is mutable.
- Parsers now ensure that the expression received is indexable using an `int`.
- Function names and pointers are now stored in array of structures instead of separate arrays.
- Rename `log` to `log10`.
- `tms_find_str_in_array()` is now aware of different types of AoS.
- Add an array containing all int function names.
- Add user defined functions to the array of regular function names.
- Add integer functions `mask()`, `inv_mask()`, `ipv4()`, `dotted()`.
- Add `tms_print_dot_decimal()`.
- `tms_print_hex()` now adds a space every 16 bit.
- `tms_print_bin()` now adds a space to separate every octet and prints all leading zeros.
- Remove the "backup" database from the error handler.
- Add support for "facilities" in the error handler.
- Error printing now includes the "facility" at which the error occured.
- Error reporting quality improvements, especially with extended and user defined functions.
- Update some header guards.

### Thread safety

- Both parsers are now thread safe (using an atomic lock and `test_and_set`)
- Evaluators are now thread safe.
- `tms_error_handler()` is now thread safe.
- Added non thread safe variants of parser and evaluators, for use only in calls from within a locked parser/evaluator.

### Fixes

- `tms_decimal_to_fraction()` no longer fails if the pattern starts with a "0".
- `integrate()` should behave properly now if the upper bound is smaller than the lower bound.
- `max()` no longer always return infinity.
-  Many fixes related to the integer masking and sign extending.
- Properly center the error indicator.


## 1.4.0 - 2024-01-13

### Changes

- Speed improvements in both parsers.
- Library initialization is now faster.
- Multiple memory safety fixes.
- Base-N value reading function now detects overflows and larger than expected values.
- Remove reliance on `tms_error_bit` and `_tms_g_expr` (on the way to thread safety).
- Fixed: The parser skips the next operand if a variable name ends with 'e' or 'E' and the operand is '+' or '-' (ex: lite+5 is treated as lite).
- Variable and runtime function creation routines now avoid shadowing.
- Exponential is now `e` instead of `exp`.
- Added `exp(x)` function, equivalent to `e^x`.
- Add `tms_get_name()` and `tms_find_str_in_array()`.
- Accuracy test is now in `libtmsolve` pipelines.

## 1.3.0 - 2023-12-23

### Added

- Functions `min`, `max` and `avg`.
- Expression dumping logic for int expressions.
- Hexadecimal, octal and binary value printing functions (moved from `tmsolve`).

### Fixed

- Possible segmentation fault in both parser variants.
- Error handler snippet generator getting confused when an error occurs in an extended function.
- Right rotation issues with 64 bit integers.

## 1.2.1 - 2023-12-16

### Changes

- Fix multiple memory access issues.
- Improve performance of `_tms_set_operand()`.
- Fix inconsistent right shift by `sr()`.
- Add `sra()` to perform arithmetic right shift.
- Indicate the column at which the error occured.
- Add function to sign extend to 64 bits.
- Sign extend runtime variables before storing them.
- Other minor changes.

## 1.2.0 - 2023-10-30

### Added: Integer Expression Mode

- Accepts input in hexadecimal, octal, binary and decimal.
- Supports most common bitwise operators.
- Supports other bitwise operations using functions.
- Can change the apparent word size using `tms_int_mask`.
- Supports user defined variables.

### Other Changes

- Runtime variable and function names can now contain digits.

## 1.1.0 - 2023-10-10

### Added

- Add user defined runtime function support.
- Add `tms_math_expr` duplication function.
- Add the string form of expressions to `tms_math_expr` struct members.
- Headers `#include` cleanup after moving most structs to a separate header.

## Fixed

- Use of uninitialized value while generating unknown members data if the expression has extended functions.
- Some memory leaks while handling extended function errors.

## 1.0.1 - 2023-9-29

First stable release of `libtmsolve`, refer to the documentation for usage.
