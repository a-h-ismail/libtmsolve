# Functions

## Introduction

This library has four function types: real, complex, extended and runtime (user defined).

- Real and complex functions take one argument and returns a value.
- Extended functions can take zero or more arguments.
- Runtime functions are created by the user during runtime, and are lost on calculator shutdown.

## Real and Complex Functions

- `fact()`: Calculate the factorial of a real value.
- `abs()`: Returns the absolute value for real numbers, or the modulus for complex numbers.
- `ceil()`: Returns the nearest integer greater or equal to the specified value. For complex values it performs the operation on the real and imaginary parts.
- `floor()`: Returns the nearest integer lesser or equal to the specified value. For complex values it performs the operation on the real and imaginary parts.
- `round()`: Rounds to the nearest integer, for complex values it performs the operation on the real and imaginary parts.
- `sign()`: Returns 1 for positive values, -1 for negative values and 0 for a zero.
- `sqrt()`: Calculates the square root.
- `cbrt()`: Calculates the cube root.
- `cos()`, `sin()`, `tan()`: Trigonometric functions, expects argument in radians.
- `acos()`, `asin()`, `atan()`: Inverse trigonometry, returns answer in radians.
- `cosh()`, `sinh()`, `tanh()`: Hyperbolic sine, cosine and tangent.
- `acosh()`, `asinh()`, `atanh()`: Inverse of the above.
- `exp(x)`: Equivalent to `e^x`.
- `ln()`: Base e logarithm.
- `log()`: Base 10 logarithm.

### Warning

Trigonometric functions lose precision with larger values. I recommend staying in range `-1099511627776;1099511627776` to keep reasonably high precision.

## Extended Functions

- `avg(v1, v2, ...)`: Calculates the average of its arguments.
- `min() and max()`: Calculates the minimun/maximum. Works only with real values.
- `integrate(start, end, function)`: Calculates integral using Simpson 3/8 rule.
- `der(function, x)`: Calculates derivative of the function at the specified point.
- `logn(value, n)`: Calculates the base-n logarithm for the specified value.
- `hex(hex_rep)`: Reads a hexadecimal representation and returns the corresponding value.
- `oct(oct_rep)`: Reads an octal representation and returns the corresponding value.
- `bin(bin_rep)`: Reads a binary representation and returns the corresponding value.
- `rand()`: Returns a random decimal value in range [INT_MIN;INT_MAX].
- `randint()`: Returns a random integer in range [INT_MIN;INT_MAX].
- `int(value)`: Returns the integer part of the value, supports complex.

## Runtime Functions

Runtime functions are created by the user during runtime. For `libtmsolve`, you have to call `tms_set_ufunction(char *name, char *function)` to add your runtime function.

## Guidelines for Adding new Extended Functions

- Function declaration should be: `double complex tms_<name>(tms_arg_list *args)`
- The extended function shouldn't free the argument list, it will be freed by the evaluator.
- Validate the argument count you get:

```C
double complex tms_foo(tms_arg_list *args)
{
    if (_validate_args_count(EXPECTED_ARGS, args->count) == false)
        return NAN;
    
    // <...>
}
```

- If you are expecting a value, read it using `tms_solve()` and handle errors properly (mainly check if the solver returns `NAN`).
- Add the function declaration to `function.h`, and the function name and pointer to `parser.c`.

Check the functions in `function.c` to see some implemented extended functions.

## Note

The names used by the parser are different from the actual names of the functions in the code. Refer to `parser.c` and `function.h` to see the actual C functions.
