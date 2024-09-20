# Integer Functions

Similar to double precision scientific mode, integer mode has functions: simple and extended.

## Simple Functions

- `abs(value)`: Returns the absolute value.
- `not(value)`: Flips all bits of `value`.
- `mask(n)`: Creates an n bit wide mask.
- `mask_bit(n)`: Creates a mask for the nth bit (LSB is considered `b0`).
- `inv_mask(n)`: Creates an n bit wide inverse mask.
- `ipv4_prefix(length)`: Generates the mask corresponding to the specified prefix length.
- `zeros(value)`: Returns the number of binary zeros in `value`.
- `ones(value)`: Returns the number of binary ones in `value`.
- `parity(value)`: Calculates the parity bit of `value`.

## Extended Functions

- `and, nand, or, nor, xor, xnor`: Expects 2 arguments, name is self explanatory.
- `rr(value, rot)`: Performs right rotation of `value` bits by `rot` bits.
- `rl(value, rot)`: Performs left rotation of `value` bits by `rot` bits.
- `sr(value, shift)`: Performs right shift of `value` bits by `shift` bits.
- `sra(value, shift)`: Performs arithmetic right shift.
- `sl(value, shift)`: Performs left shift of `value` bits by `shift` bits.
- `ipv4(a.b.c.d)`: Reads an IPv4 in dot decimal notation.
- `dotted(a.b...)`: Reads a dot decimal notation of any width.
- `mask_range(start, end)`: Generates a mask for bits in range [start, end] (LSB is considered `b0`).
- `rand(min, max)`: Returns a random value in range `[min, max]`. If arguments are omitted, default is `[0,max]`.
- `max(...)`: Finds the maximum of its arguments.
- `min(...)`: Finds the minimum of its arguments.

## Runtime Functions

Runtime functions are created by the user during runtime. Call `tms_set_int_ufunction()` to add your runtime function.
