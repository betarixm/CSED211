CSED211 Homework 1
===

# 1. Exercise 2.61
* Any bit of x equals 1.
  * `!!x`
* Any bit of x equals 0.
  * `!x`
* Any bit in the least significant byte of x equals 1.
  * `!!(x & 0xff)`
* Any bit in the most significant byte of x equals 0.
  * `!!(~x & 0xff)`
  
# 2. Exercise 2.69
```c
/*
 * Do rotating left shift. Assume 0 <= n < w
 * Examples when x = 0x12345678 and w = 32:
 *     n=4 -> 0x23456781, n=20 -> 0x67812345
 */
unsigned rotate_left(unsigned x, int n) {
    unsigned w = sizeof(unsigned) << 3;
    return (x << n) | (x >> (w - n));
}
```

# 3. Exercise 2.77
* K = 17
  * `x + (x << 4)`
* K = -7
  * `x - (x << 3)`
* K = 60
  * `(x << 6) - (x << 2)`
* K = -112
  * `(x << 4) - (x << 7)`
  
# 4. Exercise 2.83
## A.

infinite string으로 나타낸 값을 $x$라고 하자. 이때, `x << k == x + Y`이므로, $x \cdot 2^k = x + Y$ 라고 할 수 있다. 그러므로, $x$는 아래와 같이 나타낼 수 있다.

$\frac{Y}{2^k-1}$

## B.
* (a) $\frac{5}{7}$
* (b) $\frac{2}{5}$
* (c) $\frac{19}{63}$

# 5. Exercise 2.88 on page 174.
# 6. Exercise 2.89 on page 174.
# 7. Exercise 2.90 on page 175
# 8. Exercise 2.95 on page 178
Extra Points (less than 10 lines for each problem)
# 9. Describe the difference between RISC and CISC.
# 10. What is ISA (Instruction Set Architecture)?