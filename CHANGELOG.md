# Changelog

## 3.1.1 - 2026-01-31

### Fixed

- Type confusion in integer mode autocompletion helper.
- Memory leak and missing name when showing debug output in integer mode.

### Changed

- Replace stdatomic bool used as lock in the error handler with a pthread mutex.
- Improved error reporting for undefined functions and stray commas.

## 3.1.0 - 2026-01-23

### Added

- Integer functions `float`, `hamming_dist`, `multinv`, `gcd`, `lcm`.
- Long operators support: `//` and `**` in the floating point parser and `<<`, `>>`, `<<<`, `>>>`, `**` in the integer parser.
- Experimental C++/Python binder.
- Function `tms_get_last_error()`

### Changed

- Removed `=` from operators list.
- The error prefix is now always duplicated to avoid potential memory leaks.
- Many function prototypes now uses `const char*` instead of `char *`.
- Int mask setter accepts only powers of 2. 
- Always sign extend integers instead of expecting the user to do it.
- Marked all extended functions as private.

### Fixed

- Floor operation with complex values.
- Some issues caused by inconsistent masking/sign extending.

## 3.0.2 - 2025-06-15

### Fixed

- Always sign extend integer variables before storing them
- Do not handle = in tms_solve
- Memory leaks in all hashmaps during deletion and in runtime variable update

## 3.0.1 - 2024-10-23

- Improve consistency and quality of error messages while setting user functions.

## 3.0.0 - 2024-09-21

This is a major, API breaking release.

### Added

- Both parsers now support labels, used to implement multi-argument user functions.
- User defined functions for integer based expressions.
- Helper functions to generate autocompletion matches for scientific and integer modes.
- Ability to remove user defined variables and functions.
- Function to check if an array of string is made of unique strings.
- Some extra helper functions in `string_tools.h`.
- Ability to reset library state using `tmsolve_reset()`.
- Call depth limit (32) for both evaluators to avoid infinite recursion.
- Options `NO_LOCK`, `ENABLE_COMPLEX`, `PRINT_ERRORS` applicable to parsers and evaluators.
- Integer functions `min()`, `max()`, `abs()`, `parity()`.

### Changed

- Refactor the error handler, it now has its own header and six separate functions instead of one.
- Massive code redundancy cleanup in both parsers, relying on macros and `parser_common.h`
- Variables and functions now use a hashmap instead of linear arrays.
- Parsers and evaluators now accept options as flags.
- Support for multiple arguments (up to 64) in user defined functions.
- User functions are stored by name instead of pointer.
- The parser is no longer responsible for runtime variables.
- "unknowns" are now known as "labels" and no longer restricted to "x" only.
- The "prefix" string in saved errors is now stored separately instead of being concatenated with the error message.
- Locking implementation now uses `pthread` mutexes instead of `stdatomic.h` spinlocks.
- `tms_arg_list` can now carry an additional payload.
- Octal printing now adds a space every 3 digits.
- Some other refactoring.

### Removed

- A lot of linear arrays containing variables and functions.
- The old `tms_error_handler()`.

### Fixed

- Some cases where the circular and self reference checks can be bypassed, leading to infinite recursion.
- Possible out of bound access while creating an argument list if an open parenthesis is not closed.

## 2.3.0 - 2024-05-10

### Changed

- Add `rand()` to integer mode.
- Add `log2()` function.
- Drop `randint()` from scientific mode.
- `rand()` now returns a weight `[0,1]` instead of a random number in range `[0,INT_MAX]`.
- Improve clarity of extended functions errors.

### Fixed

- Parsers no longer ignore syntax errors delimited by a closing parenthesis and an operand.
- Parsing error generated in recursive calls from evaluating extended functions are no longer ignored.
- Improper error cleanup in error handler `EH_CLEAR`.
- Possible error duplication in `tms_solve()`.
- Catch and report usage of random functions in integration and derivation.
- Possible integer overflow and failure to find the fraction `in tms_decimal_to_fraction()`.
- Fix a regression introduced in the previous version that broke runtime functions circular and self reference detection.

## 2.2.0 - 2024-03-02

### Added

- Int functions `mask_bit()` and `mask_range()`.
- `tms_delete_math_expr_members()` to free memory of a math expression members without freeing the math_expr itself.

### Fixed

- When runtime defined functions are updated, the pointer to the math_struct is now conserved. This should avoid dangling pointers in other math expressions referring to the old version.

## 2.1.0 - 2024-02-25

### Changed

- Add int functions `ipv4_prefix()`, `ones()`, `zeros()`.
- Modulo operator now has the same precedence as multiplication and division in int variants.
- `tms_set_int_mask()` now enforces masks of width a power of 2.
- Catch improper shift amount in int mode shift functions.
- Catch negative rotation amount in int mode `rr()` and `rl()`.
- `ipv4()` now requires a variable width of 32 bits.

### Fixed

- Fix mask generation issue in `tms_set_int_mask()` for 64 bit width.
- Fix missing space while printing with width of 64 bits.
- Fix some incorrect error reports while reading int values.
- Fix silent error accumulation in int mode when using extended functions.
- Use correct function ID macros in int mode.
- Warn if the error database is not empty and clear it on entry of parsers/evaluators.

## 2.0.1 - 2024-02-21

### Fixed

- Fix broken behavior of `tms_print_bin()` for int masks of size different than 32.
- Fix error printer not prepending `...` when the error index is between 24 and 49.
- Fix the edge case where `+-` is treated as `+` (bug in `tms_combine_add_subtract()`).

### Added

- `tms_set_int_mask()` to set the int mask size while temporarily locking int parser and evaluator.

## 2.0.0 - 2024-02-20

This is a major, API breaking release.

### Changed

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

### Fixed

- `tms_decimal_to_fraction()` no longer fails if the pattern starts with a "0".
- `integrate()` should behave properly now if the upper bound is smaller than the lower bound.
- `max()` no longer always return infinity.
-  Many fixes related to the integer masking and sign extending.
- Properly center the error indicator.

## 1.4.0 - 2024-01-13

### Changed

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

### Changed

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
