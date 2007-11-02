/***************************************************************************

    eigccppc.h

    PowerPC (32 and 64-bit) inline implementations for GCC compilers. This
    code is automatically included if appropriate by eminline.h.

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#ifndef __EIGCCPPC__
#define __EIGCCPPC__


/***************************************************************************
    INLINE MATH FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    mul_32x32 - perform a signed 32 bit x 32 bit
    multiply and return the full 64 bit result
-------------------------------------------------*/

/* TBD */


/*-------------------------------------------------
    mulu_32x32 - perform an unsigned 32 bit x
    32 bit multiply and return the full 64 bit
    result
-------------------------------------------------*/

/* TBD */


/*-------------------------------------------------
    mul_32x32_hi - perform a signed 32 bit x 32 bit
    multiply and return the upper 32 bits of the
    result
-------------------------------------------------*/

#define mul_32x32_hi _mul_32x32_hi
INLINE INT32 _mul_32x32_hi(INT32 a, INT32 b)
{
	INT32 result;

	__asm__ (
		" mulhw  %0,%1,%2   \n"
	  : "=&r" (result),
		"+r" (val1)
	  : "r" (val2)
	  : "xer"
	);

	return result;
}


/*-------------------------------------------------
    mulu_32x32_hi - perform an unsigned 32 bit x
    32 bit multiply and return the upper 32 bits
    of the result
-------------------------------------------------*/

#define mulu_32x32_hi _mulu_32x32_hi
INLINE UINT32 _mulu_32x32_hi(UINT32 a, UINT32 b)
{
	UINT32 result;

	__asm__ (
		" mulhwu %0,%1,%2   \n"
	  : "=&r" (result),
		"+r" (val1)
	  : "r" (val2)
	  : "xer"
	);

	return result;
}


/*-------------------------------------------------
    mul_32x32_shift - perform a signed 32 bit x
    32 bit multiply and shift the result by the
    given number of bits before truncating the
    result to 32 bits
-------------------------------------------------*/

#define mul_32x32_shift _mul_32x32_shift
INLINE INT32 _mul_32x32_shift(INT32 a, INT32 b, UINT8 shift)
{
	INT32 result;

#if defined(__ppc64__) || defined(__PPC64__)
	__asm__ (
		" mulld  %0,%1,%2 \n"
		" srd    %0,%0,%3 \n"
	  : "=&r" (result)			/* result can go in any register */
	  : "%r" (val1)				/* any register, can swap with val2 */
		"r" (val2)				/* any register */
		"r" (shift)				/* any register */
	);
#else
	__asm__ (
		" mullw  %0,%2,%3   \n"
		" mulhw  %2,%2,%3   \n"
		" srw    %0,%0,%1   \n"
		" subfic %1,%1,0x20 \n"
		" slw    %2,%2,%1   \n"
		" or     %0,%0,%2   \n"
	  : "=&r" (result),
		"+r" (shift),
		"+r" (val1)
	  : "r" (val2)
	  : "xer"
	);
#endif

	return result;
}


/*-------------------------------------------------
    mulu_32x32_shift - perform an unsigned 32 bit x
    32 bit multiply and shift the result by the
    given number of bits before truncating the
    result to 32 bits
-------------------------------------------------*/

#define mulu_32x32_shift _mulu_32x32_shift
INLINE UINT32 _mulu_32x32_shift(UINT32 a, UINT32 b, UINT8 shift)
{
	UINT32 result;

#if defined(__ppc64__) || defined(__PPC64__)
	__asm__ (
		" mulldu %0,%1,%2 \n"
		" srd    %0,%0,%3 \n"
	  : "=&r" (result)			/* result can go in any register */
	  : "%r" (val1)				/* any register, can swap with val2 */
		"r" (val2)				/* any register */
		"r" (shift)				/* any register */
	);
#else
	__asm__ (
		" mullw  %0,%2,%3   \n"
		" mulhwu %2,%2,%3   \n"
		" srw    %0,%0,%1   \n"
		" subfic %1,%1,0x20 \n"
		" slw    %2,%2,%1   \n"
		" or     %0,%0,%2   \n"
	  : "=&r" (result),
		"+r" (shift),
		"+r" (val1)
	  : "r" (val2)
	  : "xer"
	);
#endif

	return result;
}


/*-------------------------------------------------
    div_64x32 - perform a signed 64 bit x 32 bit
    divide and return the 32 bit quotient
-------------------------------------------------*/

/* TBD */


/*-------------------------------------------------
    divu_64x32 - perform an unsigned 64 bit x 32 bit
    divide and return the 32 bit quotient
-------------------------------------------------*/

/* TBD */


/*-------------------------------------------------
    div_32x32_shift - perform a signed divide of
    two 32 bit values, shifting the first before
    division, and returning the 32 bit quotient
-------------------------------------------------*/

/* TBD */


/*-------------------------------------------------
    divu_32x32_shift - perform an unsigned divide of
    two 32 bit values, shifting the first before
    division, and returning the 32 bit quotient
-------------------------------------------------*/

/* TBD */


/*-------------------------------------------------
    mod_64x32 - perform a signed 64 bit x 32 bit
    divide and return the 32 bit remainder
-------------------------------------------------*/

/* TBD */


/*-------------------------------------------------
    modu_64x32 - perform an unsigned 64 bit x 32 bit
    divide and return the 32 bit remainder
-------------------------------------------------*/

/* TBD */


/*-------------------------------------------------
    recip_approx - compute an approximate floating
    point reciprocal
-------------------------------------------------*/

/* TBD */



/***************************************************************************
    INLINE BIT MANIPULATION FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    count_leading_zeros - return the number of
    leading zero bits in a 32-bit value
-------------------------------------------------*/

#define count_leading_zeros _count_leading_zeros
INLINE UINT8 _count_leading_zeros(UINT32 value)
{
	UINT32 result;

	__asm__ (
		" cntlzw %0,%1 \n"
		: "=r" (result)		/* result can be in any register */
		: "r" (value)		/* 'value' can be in any register */
	);

	return result;
}


/*-------------------------------------------------
    count_leading_ones - return the number of
    leading one bits in a 32-bit value
-------------------------------------------------*/

#define count_leading_ones _count_leading_ones
INLINE UINT8 _count_leading_ones(UINT32 value)
{
	UINT32 result;

	__asm__ (
		" not %0,%1 \n"
		" cntlzw %0,%0 \n"
		: "=r" (result)		/* result can be in any register */
		: "r" (value)		/* 'value' can be in any register */
	);

	return result;
}



/***************************************************************************
    INLINE SYNCHRONIZATION FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    compare_exchange32 - compare the 'compare'
    value against the memory at 'ptr'; if equal,
    swap in the 'exchange' value. Regardless,
    return the previous value at 'ptr'.
-------------------------------------------------*/

#define compare_exchange32 _compare_exchange32
INLINE INT32 _compare_exchange32(INT32 volatile *ptr, INT32 compare, INT32 exchange)
{
	register INT32 result;

	__asm__ __volatile__ (
		"1: lwarx  %[result], 0, %[ptr]   \n"
		"   cmpw   %[compare], %[result]  \n"
		"   bne    2f                     \n"
		"   sync                          \n"
		"   stwcx. %[exchange], 0, %[ptr] \n"
		"   bne- 1b                       \n"
		"2:                                 "
		: [result]   "=&r" (result)
		: [ptr]      "r"   (ptr)
		, [exchange] "r"   (exchange)
		, [compare]  "r"   (compare)
		: "cr0"
	);

	return result;
}


/*-------------------------------------------------
    compare_exchange64 - compare the 'compare'
    value against the memory at 'ptr'; if equal,
    swap in the 'exchange' value. Regardless,
    return the previous value at 'ptr'.
-------------------------------------------------*/

#ifdef PTR64
#define compare_exchange64 _compare_exchange64
INLINE INT64 _compare_exchange64(INT64 volatile *ptr, INT64 compare, INT64 exchange)
{
	register INT64 result;

	__asm__ __volatile__ (
		"1: ldarx  %[result], 0, %[ptr]   \n"
		"   cmpd   %[compare], %[result]  \n"
		"   bne    2f                     \n"
		"   sync                          \n"
		"   stdcx. %[exchange], 0, %[ptr] \n"
		"   bne--  1b                     \n"
		"2:                                 "
		: [result]   "=&r" (result)
		: [ptr]      "r"   (ptr)
		, [exchange] "r"   (exchange)
		, [compare]  "r"   (compare)
		: "cr0"
	);

	return result;
}
#endif


/*-------------------------------------------------
    atomic_exchange32 - atomically exchange the
    exchange value with the memory at 'ptr',
    returning the original value.
-------------------------------------------------*/

#define atomic_exchange32 _atomic_exchange32
INLINE INT32 _atomic_exchange32(INT32 volatile *ptr, INT32 exchange)
{
	register INT32 result;

	__asm__ __volatile__ (
		"1: lwarx  %[result], 0, %[ptr]   \n"
		"   sync                          \n"
		"   stwcx. %[exchange], 0, %[ptr] \n"
		"   bne-   1b                     \n"
		: [result]      "=&r" (result)
		: [ptr]      "r"   (ptr)
		, [exchange] "r"   (exchange)
		: "cr0"
	);

	return result;
}


/*-------------------------------------------------
    atomic_add32 - atomically add the delta value
    to the memory at 'ptr', returning the final
    result.
-------------------------------------------------*/

#define atomic_add32 _atomic_add32
INLINE INT32 _atomic_add32(INT32 volatile *ptr, INT32 delta)
{
	register INT32 result;

	__asm __volatile__ (
		"1: lwarx  %[result], 0, %[ptr]     \n"
		"   add    %[result], %[result], %[delta] \n"
		"   sync                            \n"
		"   stwcx. %[result], 0, %[ptr]     \n"
		"   bne- 1b                         \n"
		: [result]   "=&b" (result)
		: [ptr]   "r"   (ptr)
		, [delta] "r"   (delta)
		: "cr0"
	);

	return result;
}


#endif /* __EIGCCPPC__ */
