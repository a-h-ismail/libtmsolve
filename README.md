# Introduction

libtmsolve is a math library for expression evaluation (both scientific and integer based), basic manipulation of matrices and some other useful features (function manipulation, factorization...).

Full documentation is available [here](https://a-h-ismail.gitlab.io/libtmsolve-docs/).

## Features

- Thread safe and minimal.
- Doesn't need any additional dependency.
- Parse and evaluate both regular (floating point) and integer based expressions.
- Integer parser supports varying the word size (up to 64 bit) using a global integer mask.
- Supports variables and functions defined at runtime.
- Basic matrix manipulation.
- Integer factorization and function reduction.
- Convert a value to its fractional form (if possible).

## Usage Examples

As a normal calculator:

```C
#include <tmsolve/libtmsolve.h>
// <...> Whatever function you are in
double answer;
char light_speed[] = {"sqrt(1/(8.8541878128e-12*(4e-7*pi)))"};

// Expected answer: 299792458.08161
answer = tms_solve(light_speed);

// By default, tms_solve() will switch to complex calculations if no real answer is found.
// If this behavior is undesirable, use the following variant with enable_complex set to false
answer = tms_solve_e(light_speed, false);
```

With unknown operands:

```C
#include <tmsolve/libtmsolve.h>
// <...> Whatever function you are in
char expr_with_unknowns[] = {"x^2+2*x+4"};
// First step is to parse the expression and generate a tms_math_expr
tms_math_expr *M = tms_parse_expr(expr_with_unknowns, true, false);

// Check if M is null (errors are automatically sent to stderr)
if(M == NULL)
{
    //<...>
}

// Set the desired variable value, for example 12
tms_set_unknown(M, 12);

// Solve M for the value you just set
answer = tms_evaluate(M);

// Don't leak memory
tms_delete_math_expr(M);
```

With integer variables:

```C
#include <tmsolve/libtmsolve.h>
// <...> Whatever function you are in
char int_expression[] = {"821 & 0xFF + 0b1 | 0b110"};
int64_t result;

// The return value reports if the operation succeeded or not
status = tms_int_solve(int_expression, &result);

// You can also use the parser then evaluator like the double precision counterpart, but no need.

```

## Linking to the Library

To build your binary and link to this library:
`gcc -ltmsolve <other options>`

## Tips

Documentation about the parser, evaluator, and some useful features like factorization can be found in `parser.h`, `evaluator.h` and `scientific.h`.

Integer variant of the parser is in `int_parser.h`, its extended functions are in `bitwise.h`.

For functions that assist in manipulating strings (char *), refer to `string_tools.h`.

For matrix related features, refer to `matrix.h`.

For functions used internally to manage the calculator (like error handling), refer to `internals.h`.

All error messages are defined in `m_errors.h`.

Extended functions (like integration) are defined in `function.h`.

## Note

The library uses double precision floating point variables with complex extension for its operations.

## License

This library is licensed under the GNU LGPL-2.1-only.
