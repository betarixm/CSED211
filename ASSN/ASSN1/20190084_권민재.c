/* 
 * CS:APP Data Lab 
 * 
 * 권민재 mzg00
 * 
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting an integer by more
     than the word size.

EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implent floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operators (! ~ & ^ | + << >>)
     that you are allowed to use for your implementation of the function. 
     The max operator count is checked by dlc. Note that '=' is not 
     counted; you may use as many of these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */


#endif
/* Copyright (C) 1991-2018 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */
/* This header is separate from features.h so that the compiler can
   include it implicitly at the start of every compilation.  It must
   not itself include <features.h> or any other header that includes
   <features.h> because the implicit include comes before any feature
   test macros that may be defined in a source file before it first
   explicitly includes a system header.  GCC knows the name of this
   header in order to preinclude it.  */
/* glibc's intent is to support the IEC 559 math functionality, real
   and complex.  If the GCC (4.9 and later) predefined macros
   specifying compiler intent are available, use them to determine
   whether the overall intent is to support these features; otherwise,
   presume an older compiler has intent to support these features and
   define these macros by default.  */
/* wchar_t uses Unicode 10.0.0.  Version 10.0 of the Unicode Standard is
   synchronized with ISO/IEC 10646:2017, fifth edition, plus
   the following additions from Amendment 1 to the fifth edition:
   - 56 emoji characters
   - 285 hentaigana
   - 3 additional Zanabazar Square characters */
/* We do not support C11 <threads.h>.  */
/* 
 * bitOr - x|y using only ~ and & 
 *   Example: bitOr(6, 5) = 7
 *   Legal ops: ~ &
 *   Max ops: 8
 *   Rating: 1
 */
int bitOr(int x, int y) {
    /* 드모르간의 법칙을 이용하면 된다. */
    return ~(~x & ~y);
}

/*
 * addOK - Determine if can compute x+y without overflow
 *   Example: addOK(0x80000000,0x80000000) = 0,
 *            addOK(0x80000000,0x70000000) = 1, 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 3
 */
int addOK(int x, int y) {
    /*
     * x와 y의 부호가 다른 경우,
     * 그리고, x와 y의 부호가 같은데 x와 y를 더한 결과의 부호와 x의 부호가 같다면,
     * overflow 없이 더할 수 있는 경우라고 말할 수 있다.
     */
    int is_x_y_differ = x ^ y /* x와 y의 부호가 다른 경우 */ ;
    int is_sum_and_x_same = ~((x + y) ^ x) /*x와 y를 더한 결과와 x의 부호가 같은 경우*/;
    return (( is_x_y_differ | is_sum_and_x_same ) >> 31) & 1; /* 부호 비트만 추출해서 반환 */
}

/*
 * negate - return -x 
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x) {
    /* not을 이용하여 1의 보수를 취한 다음 1을 더하면 negate 할 수 있다. */
    return ~x + 1;
}

/*
 * logicalShift - shift x to the right by n, using a logical shift
 *   Can assume that 0 <= n <= 31
 *   Examples: logicalShift(0x87654321,4) = 0x08765432
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 3 
 */
int logicalShift(int x, int n) {
    /* 우리가 원하는 부분만 자르기 위한 필터를 만들고 적용시켜 구현한다. */
    int mask = ((1 << 31) >> n) << 1; /* 제일 큰 자리 수 부터 n 개의 비트가 1이 되도록 하는 마스크를 만든다. */
    return (~mask) & (x >> n); /* mask에 not을 붙여서 우리가 원하는 필터를 만들고, 이를 x >> n (arithmetic shift)한 결과에 and를 통해 적용시킨다. */
}

/*
 * bitCount - returns count of number of 1's in word
 *   Examples: bitCount(5) = 2, bitCount(7) = 3
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 40
 *   Rating: 4
 */
int bitCount(int x) {
    /*
     * 2비트씩 묶어서 1의 개수를 세고,
     * 해당 정보를 바탕으로 4비트 안의 1의 개수, 8비트 안의 1의 개수, 16비트 안의 1의 개수, 32비트 안의 1의 개수를 순서대로 구한다.
     * 2비트 안의 1의 개수를 구할 때에는, x의 홀수 번째 비트와 (x >> 1)의 홀수번째 비트를 더함으로써,
     * 그 결과를 2비트씩 묶으면 2비트 안의 1의 개수가 되도록 한다.
     * 이 이후, 4비트 안의 1의 개수를 구할 때에는, 위의 결과와 (위의 결과 >> 2)의 4n+1, 4n+2번째 비트를 더함으로써,
     * 그 결과를 4비트씩 묶으면 4비트 안의 1의 개수가 되도록 한다.
     * 이러한 과정을 word size 안의 1의 개수를 구할 때 까지 반복한다.
     */

    int mask_16 = (0xff << 8) ^ 0xff;       /* 32n + 1, ... , 32n + 16 번째 비트 마스크  */
    int mask_08 = (mask_16 << 8) ^ mask_16; /* 16n + 1, ... , 16n + 8번째 비트 마스크 */
    int mask_04 = (mask_08 << 4) ^ mask_08; /*  8n + 1, ... , 8n + 4번째 비트 마스크 */
    int mask_02 = (mask_04 << 2) ^ mask_04; /*  4n + 1, 4n + 2번째 비트 마스크 */
    int mask_01 = (mask_02 << 1) ^ mask_02; /*  2n + 1번째 비트 마스크 */

    x = (x & mask_01) + ((x >> 1) & mask_01);     /*  2비트 씩 묶어서 셌을 때, 2비트 안의 1의 개수 */
    x = (x & mask_02) + ((x >> 2) & mask_02);     /*  4비트 안의 1의 개수 */
    x = (x & mask_04) + ((x >> 4) & mask_04);     /*  8비트 안의 1의 개수 */
    x = (x & mask_08) + ((x >> 8) & mask_08);     /* 16비트 안의 1의 개수 */
    return (x & mask_16) + ((x >> 16) & mask_16); /* 32비트 안의 1의 개수 */
}

/*
 * float_neg - Return bit-level equivalent of expression -f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representations of
 *   single-precision floating point values.
 *   When argument is NaN, return argument.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 10
 *   Rating: 2
 */
unsigned float_neg(unsigned uf) {
    /*
     * NaN이 아닐 경우에 부호 비트를 바꿔서 반환함으로써 negate한다.
     */
    if ((0x7F800000 & uf) == 0x7F800000 && (0x7FFFFF & uf)) {
        /* NaN일 경우에 uf를 그대로 반환한다. */
        return uf;
    } else {
        /* NaN이 아닐 경우에 부호 비트를 바꿔서 반환한다. */
        return uf ^ 0x80000000;
    }
}

/*
 * float_i2f - Return bit-level equivalent of expression (float) x
 *   Result is returned as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point values.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_i2f(int x) {
    int sign = x & 0x80000000;
    int exp = 1;
    int mantissa = 0;
    int round = 0;
    if (x == 0) {
        return 0;
    }

    if (x == 0x80000000) {
        return 0xcf000000;
    }

    if (sign) { /* x가 음수인 경우 우선 양수로 바꾼다. */
        x = -x;
    }

    /* 지수의 크기 (exp)을 구한다. */
    while (((x >> exp) != 0) && exp++);
    exp--;

    /* 가수부(mantissa)을 구한다. */
    mantissa = (x << (31 - exp)) & 0x7fffffff;

    /* 반올림 조건을 위해 맨 아래 한 바이트를 가져온다. */
    round = mantissa & 0xff;

    /* 반올림을 할 조건의 대상이 되는 수를 구한 후, 8자리를 쉬프트 해서 가수부 자리로 옮긴다.*/
    mantissa = mantissa >> 8;

    /* 만약, 지수의 크기가 23보다 크고, 반올림을 해야할 조건이라면, rounding 한다.*/
    if ((exp > 23) && (round > 0x80 || ((round == 0x80) && (mantissa & 1)))) {
        mantissa++;
    }

    /* 부호비트 + 지수부 + 가수부의 결과를 반환한다. */
    return sign + ((exp + 127) << 23) + mantissa;

}

/*
 * float_twice - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_twice(unsigned uf) {
    /*
     * 각 경우에 따라서 지수부나 가수부를 조작하여 2배를 만든다.
     */
    /* 지수부를 추출한다. */
    int exp = uf & 0x7f800000;
    if (exp == 0x7f800000) {
        /* NaN인 경우에 그대로 반환한다. */
        return uf;
    } else if (exp == 0) {
        /* 지수부가 0일 경우에는, 주어진 수를 왼쪽으로 쉬프트 한 후, 부호비트를 붙여서 반환하면 된다. */
        return (uf & 0x80000000) | (uf << 1);
    }
    /* 나머지의 경우에는, 지수부를 1 증가시키면 2배가 되므로, 그것을 반환하면 된다. */
    return uf + 0x800000;
}