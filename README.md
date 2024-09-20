# Introduction

libtmsolve is a math library for expression evaluation (both scientific and integer based), basic manipulation of matrices and some other useful features (function manipulation, factorization...).

Full documentation is available [here](https://a-h-ismail.gitlab.io/libtmsolve-docs/).

## Features

- Thread safe and minimal.
- Needs almost no dependency.
- Parse and evaluate both regular (floating point) and integer based expressions.
- Integer parser supports varying the word size (up to 64 bit) using a global integer mask.
- Supports variables and functions defined at runtime.
- Basic matrix manipulation.
- Integer factorization and fraction reduction.
- Convert a value to its fractional form (if possible).
- Suitable for interactive use with its support for user variables and functions.

## Usage Examples

As a normal calculator:

```C
#include <tmsolve/libtmsolve.h>
// <...> Whatever function you are in
double answer;
char light_speed[] = {"sqrt(1/(8.8541878128e-12*(4e-7*pi)))"};

// Expected answer: 299792458.08161
answer = tms_solve(light_speed);

// By default, tms_solve() will switch to complex calculations if no real answer is found, and will print errors to stderr

// This variant provides more control using options (NO_LOCK, ENABLE_COMPLEX, PRINT_ERRORS)
// 0 means defaults (use locks, no error printing, no complex)
answer = tms_solve_e(light_speed, 0,NULL);
```

With integer variables:

```C
#include <tmsolve/libtmsolve.h>
// <...> Whatever function you are in
char int_expression[] = {"821 & 0xFF + 0b1 | 0b110"};
int64_t result;

// The return value reports if the operation succeeded or not
status = tms_int_solve(int_expression, &result);

// Similarly to the double variant, you can use the more advanced tms_int_solve_e
status = tms_int_solve(int_expression, &result, 0, NULL);
```

Check the relevant documentation for more details.

## Linking to the Library

To build your binary and link to this library:
`gcc -ltmsolve <other options>`

## Tips

- Documentation about the parser, evaluator, and some useful features like factorization can be found in `parser.h`, `evaluator.h` and `scientific.h`.
- Extended functions (like integration) are defined in `function.h`.
- Integer variant of the parser is in `int_parser.h`, its extended functions are in `bitwise.h`.
- For functions that assist in manipulating strings (char *), refer to `string_tools.h`.
- For matrix related features, refer to `matrix.h`.
- Error handling is defined in `error_handler.h`.
- All error messages are defined in `m_errors.h`.

## Note

The library uses double precision floating point variables with complex extension for its "scientific" operations,
and uses `int64_t` for the integer only version of the parser and evaluator.

## License

This library is licensed under the GNU LGPL-2.1-only.
