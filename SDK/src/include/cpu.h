/*
 * Copyright: (c) 2006-2007, 2008 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:        sys.c
 * Purpose:     System level functions.
 * History:     01/15/2007, created by andyzhou
 */

#ifndef __CPU_H__
#define __CPU_H__

#include "types.h"

#if defined(D233L)
#include <xtensa/hal.h>
#include <xtensa/tie/xt_core.h>
#include <xtensa/config/specreg.h>
#include <xtensa/config/core.h>
#include <xtensa/xtruntime.h>
#endif

#if defined(FPGA_VERSION)
#define PLL_CLK                 (50000000)
#elif defined(UNICORN2M) || defined(UNICORN8M)
#define PLL_CLK                 (200000000)
#else
#define PLL_CLK                 (400000000)
#endif

extern uint32_t CPU_CLK;

#define CACHE_SIZE     		0x4000
#define CACHE_LINE_SIZE		32
#define CACHE_ALIGNED_BUFSZ(sz)	((sz) > CACHE_LINE_SIZE ? (sz) : CACHE_LINE_SIZE)
#define __CACHEALIGNED		__attribute__((aligned(CACHE_LINE_SIZE)))

#if defined(FPGA_VERSION) || defined(LIGHTELF2M) || defined(UNICORN2M)
#define SIZE_OF_SDRAM		0x200000
#define SIZE_OF_LRAM		0x10000
#elif defined(LIGHTELF8M) || defined(UNICORN8M)
#define SIZE_OF_SDRAM		0x800000
#define SIZE_OF_LRAM		0x10000
#elif defined(ROLAND1M)
#define SIZE_OF_SDRAM		0x000000
#define SIZE_OF_LRAM		0x100000
#elif defined(ROLAND1_1M)
#define SIZE_OF_SDRAM		0x000000
#define SIZE_OF_LRAM		0x120000
#elif defined(MIZAR1M)
#define SIZE_OF_SDRAM		0x000000
#define SIZE_OF_LRAM		0x140000
#elif defined(ROLAND9_1M)
#define SIZE_OF_SDRAM		0x800000
#define SIZE_OF_LRAM		0x120000
#elif defined(MIZAR9M)
#define SIZE_OF_SDRAM		0x800000
#define SIZE_OF_LRAM		0x140000
#elif defined(VENUS2M)
#define SIZE_OF_SDRAM		0x000000
#define SIZE_OF_LRAM		0x200000
#elif defined(VENUS8M)
#define SIZE_OF_SDRAM		0x800000
#define SIZE_OF_LRAM		0x200000
#else
#error "Build Properties, Missing ASIC_VERSION or FPGA_VERSION ..."
#endif

#define START_OF_SDRAM_CPU	0x0
#define START_OF_LRAM_CPU	0x20000000
#define END_OF_SDRAM_CPU	(START_OF_SDRAM_CPU + SIZE_OF_SDRAM)
#define END_OF_LRAM_CPU		(START_OF_LRAM_CPU + SIZE_OF_LRAM)

#define START_OF_VLRAM_CPU	0x40000000
#define END_OF_VLRAM_CPU	(START_OF_VLRAM_CPU + SIZE_OF_LRAM)

#define START_OF_SDRAM_DMA	0xA0000000
#define START_OF_LRAM_DMA	0x10000000
#define END_OF_SDRAM_DMA	(START_OF_SDRAM_DMA + SIZE_OF_SDRAM)
#define END_OF_LRAM_DMA		(START_OF_LRAM_DMA + SIZE_OF_LRAM)

#define CPU_CYCLE_PER_US	(CPU_CLK / 1000000)
#define CPU_CYCLE_PER_MS	(CPU_CLK / 1000)
#define OS_TICKS_PER_SEC	100
#define MS_PER_OS_TICKS		(1000 / OS_TICKS_PER_SEC)

#define TIMER0_INTERVAL		(CPU_CLK / OS_TICKS_PER_SEC)


#if defined(D233L)
#define OS_ENTER_CRITICAL()				\
({							\
	uint32_t tmp;					\
							\
	tmp = XT_RSIL(5);				\
	XT_ESYNC();					\
							\
	tmp;						\
})

#define OS_EXIT_CRITICAL(cpu_sr)	do {		\
	XT_WSR_PS(cpu_sr);				\
	XT_ESYNC();					\
} while(0)

#elif defined(RISCV)
#define OS_ENTER_CRITICAL()				\
({							\
	uint32_t tmp;					\
	__asm__ volatile("csrrci %0, mstatus, 8\n\t"	\
		     : "=r" (tmp)			\
		     :					\
		     : "memory");			\
	tmp;						\
})

#define OS_EXIT_CRITICAL(cpu_sr)	do {		\
	__asm__ volatile("csrw mstatus, %0\n\t"		\
		     :					\
		     : "r" (cpu_sr)			\
		     : "memory");			\
} while(0)

#elif defined(ARMCM33)
#define OS_ENTER_CRITICAL()				\
({							\
	uint32_t tmp;					\
	__asm__ volatile("mrs %0, faultmask\n\t"	\
		     "cpsid f\n\t"			\
		     "isb\n\t"				\
		     : "=r" (tmp)			\
		     :					\
		     : "memory");			\
	tmp;						\
})

#define OS_EXIT_CRITICAL(cpu_sr)	do {		\
	__asm__ volatile("msr faultmask, %0\n\t"	\
		     "isb\n\t"				\
		     :					\
		     : "r" (cpu_sr)			\
		     : "memory");			\
} while(0)
#endif

extern void cpu_set_interrupt_handler(int32_t idx, void (*h)(int32_t idx));
extern uint32_t cpu_get_cycle(void);
extern void cpu_idle(void);


/* Theoretically, an atomic implementation doesn't involve any operations of disabling or
 * enabling interrupts, since most modern architecture either directly support a set of
 * atomic instructions or provide an operation to lock the memory bus for a set of instructions.
 * However, we choose to implement an atomic operation in an interrupt disabling way due to the
 * fact that the atomic is subsequently used to build the spin lock which is heavily used to 
 * protect critical regions. Particularly, it's possible for an interrupt handler also to use a
 * spin lock if it has a critical region against some tasks. Therefore, the local interrutps
 * must be disabled before obtaining the lock. Otherwise, the interrupt handler may interrupt
 * the task who has already held the lock and then spin, waiting for the lock to become
 * available which is impossible because the lock holder has no opportunity to run until the 
 * interrupt handler completes. A dead lock occurs. Be short, atomic is roughly equivalent to
 * interrupt disabling for an unique processor system.
 */
typedef int32_t	atomic_t;

/* atomic_set - Set atomic variable
 */
static __inline__ void atomic_set(atomic_t *a, uint32_t i)
{
	uint32_t cpu_sr;

	cpu_sr = OS_ENTER_CRITICAL();
	*a = i;
	OS_EXIT_CRITICAL(cpu_sr);
	return;
}
/* atomic_inc - Increment atomic variable
 */
static __inline__ atomic_t atomic_inc(atomic_t *a)
{
	uint32_t cpu_sr;
	atomic_t old;

	cpu_sr = OS_ENTER_CRITICAL();
	old = (*a)++;
	OS_EXIT_CRITICAL(cpu_sr);

	return old;
}
/* atomic_dec - Decrement atomic variable
 */
static __inline__ atomic_t atomic_dec(atomic_t *a)
{
	uint32_t cpu_sr;
	atomic_t old;

	cpu_sr = OS_ENTER_CRITICAL();
	old = (*a)--;
	OS_EXIT_CRITICAL(cpu_sr);

	return old;
}
/* atomic_dec_and_test - Decrement and test, returns true if the result is 0,
 *	or false for all other cases.
 */
static __inline__ uint32_t atomic_dec_and_test(atomic_t *a)
{	
	uint32_t cpu_sr, ret;
	
	cpu_sr = OS_ENTER_CRITICAL();
	--(*a);
	ret = *a ? 0 : 1;
	OS_EXIT_CRITICAL(cpu_sr);

	return ret;
}
/* atomic_test_and_dec - Test then decrement if it can be, return true if decremented
 *	or false otherwise.
 */
static __inline__ uint32_t atomic_test_and_dec(atomic_t *a)
{
	uint32_t cpu_sr;
	
	cpu_sr = OS_ENTER_CRITICAL();
	if (*a) {
		--(*a);
		OS_EXIT_CRITICAL(cpu_sr);
		return 1;
	}

	OS_EXIT_CRITICAL(cpu_sr);
	return 0;
}
/* spin_lock - 
 *
 */
static __inline__ void spin_lock(atomic_t *a)
{
	while (!atomic_test_and_dec(a))
		;
}
/* spin_unlock - 
 *
 */
static __inline__ void spin_unlock(atomic_t *a)
{
	atomic_inc(a);
}


/*
 *      These inlines deal with timer wrapping correctly. You are
 *      strongly encouraged to use them
 *      1. Because people otherwise forget
 *      2. Because if the timer wrap changes in future you won't have to
 *         alter your driver code.
 *
 * time_after(a,b) returns true if the time a is after time b.
 *
 * Do this with "<0" and ">=0" to only test the sign of the result. A
 * good compiler would generate better code (and a really good compiler
 * wouldn't care). Gcc is currently neither.
 *
 *	Note: a, b shall both be unsigned long integers. a shall not be
 * far too much from b.
 * 
 * Proof:
 *
 * M   - the complementation constant for a 2's complement number system with K digits,
 *                       k
 *       whose value is 2.
 *
 * MAX - the maximum positive value in a 2's complement number system with K digits,
 *                   k-1
 *        which is  2  -  1 = M/2 - 1.
 *
 *      time movement direction
 *               -->
 *               MAX              M
 * +----+--+------+--------------++  if 0 < b < a, time a doesn't go too far from time b, obviously time a is after time b.
 * 0    b  a                    -1
 *               MAX              M
 * +----+---+-----+-----+--------++  if b > 0 and a < 0, b - a = b - (-|a|) = b + |a|, pay attention to the value of |a|:
 * 0    b  |a|          a       -1   much further is time a from time b, less value does |a| has;
 *                                   much closer is time a to time b, more value does |a| has.
 *                                     suppose (b + |a|) <= MAX, that is, (b + |a|) > 0. More precisely,
 *                                       b + |a| = b + M - a = M - (a - b) <= MAX = M/2 - 1, namely, a - b - 1 >= M/2, and
 *                                       a - b > M/2,
 *                                       wherein the left part of the inequality implies the distance from a to b, thus,
 *                                       if time a goes more than M/2 further from time b, time a won't be considered to be
 *                                       after time b.
 *                                     suppose (b + |a|) > MAX, that is, (b + |a|) < 0, in such case, time a is considered
 *                                       to be after time b.
 *               MAX              M
 * +--+-+---------+--------------++  if b > a > 0, time a goes too much far (larger than M/2) from time b, obviously time a
 * 0  a b                       -1   is not after time b.
 *
 *
 *               MAX              M
 * ---------------+-------+---+--++  if b < a < 0, time a doesn't go too far from time b, obviously time a is after time b.
 *                        b   a -1
 *               MAX              M
 * +--+---+-------+-------+------++  if b < 0 and a > 0, b - a = -|b| - a = -(|b| + a), pay attention to the value of a:
 * 0  a  |b|              b     -1   much further is time a from time b, more value does a has;
 *                                   much closer is time a to time b, less value does a has.
 *                                     suppose (|b| + a) > MAX, that is, (|b| + a) < 0 and -(|b| + a) > 0. More precisely,
 *                                       |b| + a = M - b + a > MAX = M/2 - 1, namely, M + a - b > M/2,
 *                                       wherein the left part of the inequality implies the distance from a to b, thus,
 *                                       if time a goes more than M/2 further from time b, time a won't be considered to be
 *                                       after time b.
 *                                     suppose (|b| + a) <= MAX, that is, (|b| + a) > 0 and -(|b| + a) < 0, in such case,
 *                                       time a is considered to be after time b.
 *               MAX              M
 * +--------------+----+--+------++  if 0 > b > a, a goes too much far (larger than M/2) from b, obviously time a is not
 * 0                   a  b     -1   after time b.
 */
#ifndef time_after
#define time_after(a,b)		((long)(b) - (long)(a) < 0)
#endif

#ifndef time_before
#define time_before(a,b)	time_after(b,a)
#endif

#ifndef time_after_eq
#define time_after_eq(a,b)	((long)(a) - (long)(b) >= 0)
#endif

#ifndef time_before_eq
#define time_before_eq(a,b)	time_after_eq(b,a)
#endif

#ifndef time_diff
#define time_diff(a,b)		((unsigned long)(a) - (unsigned long)(b))
#endif

extern int32_t cpu_freq_shift(uint32_t div);

#define SPM_LOW     0
#define SPM_HIGH    1
extern int32_t system_power_mode_set(uint32_t mode);


#endif	/* end of __CPU_H__ */
