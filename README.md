# Introduction

libtmsolve is a math library for expression evaluation, basic manipulation of matrices and some other useful features (function manipulation, factorization...).

## Basic use

To simply evaluate an expression and get the answer:

```C
#include <tmsolve/libtmsolve.h>
double answer;
// Here we calculate light speed in the vaccum as an example:
char light_speed[]={"sqrt(1/(8.8541878128e-12*(4e-7*pi)))"};
// Expected answer: 299792458.08161
answer = calculate_expr_auto(light_speed);
```

To build your binary and link to this library:
`gcc -ltmsolve <other options>`

Full documentation is available [here](https://a-h-ismail.gitlab.io/libtmsolve-docs/).

## Tips

Documentation about the parser, evaluator, and some useful features like factorization can be found in `scientific.h`.

For matrix related features, refer to `matrix.h`.

For functions used internally to manage the calculator (like error handling), refer to `internals.h`.

All error messages are defined in `m_errors.h`.

For functions that assist in manipulating strings (char *), refer to `string_tools.h`.

For functions used to manipulate variables and the calculator functions, refer to `function.h`.

## Note

The library uses double precision floating point variables with complex extension for its operations.

## License

This library is licensed under the GNU LGPL-2.1-only.
