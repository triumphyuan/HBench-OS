/*
 * Copyright (c) 1997 The President and Fellows of Harvard College.
 * All rights reserved.
 * Copyright (c) 1997 Aaron B. Brown.
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program, in the file COPYING in this distribution;
 *   if not, write to the Free Software Foundation, Inc., 675 Mass Ave,
 *   Cambridge, MA 02139, USA.
 *
 * Results obtained from this benchmark may be published only under the
 * name "HBench-OS".
 */

/*
 * cyclecounter.c -- interface to hardware cycle counters for the i386
 *
 * $Id: cyclecounter.c,v 1.3 1997/06/27 00:35:25 abrown Exp $
 */

/*
 * These are very architecture specific. For each architecture, the following
 * must be present
 *
 *	* a typedef for clk_t, the data type used to represent clock values
 *	  externally--should be something that the compiler can use without
 *	  choking (i.e. gcc 2.4.5 won't use register variables anywhere in 
 *	  a function when you pass 64-bit types around on the x86)
 *	* a typedef for internal_clk_t, the data type which represents clock
 *	  values internally. This should support the full resolution of the
 *	  timer (e.g. 64 bits on the x86)
 *	* a required function 
 *		void read_counter(internal_clk_t *rtnval)
 *	* a non-required function
 *		void zero_counter(void)
 */

#if defined(i386) && defined(__GNUC__)

/* must define u_int64_t */
#if defined(__bsdi__)
#include <machine/types.h>
#else
#include <sys/types.h>
#endif

/*
 * This requires, but cannot check for from user level, a Pentium or newer
 * processor supporting the RDTSC instruction.
 * The assembly also requires gcc.
 * We also need %qd support in printf for printing quads, and a strtoq
 * instruction for the TCP bandwidth test.
 */

typedef u_int32_t clk_t;	/* time stamps are 64 bit on the Pentium Pro */
typedef u_int64_t internal_clk_t;
#define CLKTFMT		"%d"
#define CLKTSTR(x) 	((int)strtol(x, NULL, 10))

/* 
 * Read function: read hardware timestamp (cycle) counter 
 */
#define read_cycle_counter(res_p) \
  __asm __volatile (						\
	 ".byte 0xf; .byte 0x31	# RDTSC instruction		\n\
         movl %%edx, %1		# High order 32 bits		\n\
         movl %%eax, %0		# Low order 32 bits"		\
	: "=g" (*(int *)(res_p)), "=g" (*(((int *)res_p)+1)) 	\
	: /* no input regs */					\
	: "eax", "edx")

/*
 * Zero function: can't implement from user level
 */
void
zero_cycle_counter()
{
	return;
}

#else /* i386 */
#error Cycle counter support requires a Pentium or Pentium Pro, and gcc
#endif
