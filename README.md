# Introduction

libtmsolve is a math library for expression evaluation, basic manipulation of matrices and some other useful features (function manipulation, factorization...).

Full documentation is available [here](https://a-h-ismail.gitlab.io/libtmsolve-docs/).

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

// If any error occured, print the errors using the error handler
tms_error_handler(EH_PRINT);
```

With unknown operands:

```C
#include <tmsolve/libtmsolve.h>
// <...> Whatever function you are in
char expr_with_unknowns[] = {"x^2+2*x+4"};
// First step is to compile the expression into a tms_math_expr
tms_math_expr *M = tms_parse_expr(expr_with_unknowns, true, false);

// Check if M is NULL and call error handler if you want (not included in the example)

// Set the desired variable value, for example 12
tms_set_unknown(M, 12);

// Solve M for the value you just set
answer = tms_evaluate(M);
```

## Linking to the Library

To build your binary and link to this library:
`gcc -ltmsolve <other options>`

## Tips

Documentation about the parser, evaluator, and some useful features like factorization can be found in `parser.h`, `evaluator.h` and `scientific.h`.

For functions that assist in manipulating strings (char *), refer to `string_tools.h`.

For matrix related features, refer to `matrix.h`.

For functions used internally to manage the calculator (like error handling), refer to `internals.h`.

All error messages are defined in `m_errors.h`.

Extended functions (like integration) are defined in `function.h`.

## Note

The library uses double precision floating point variables with complex extension for its operations.

## License

This library is licensed under the GNU LGPL-2.1-only.
