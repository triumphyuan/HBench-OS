/*-
 * Copyright (c) 1997 The President and Fellows of Harvard College.
 * All rights reserved.
 * Copyright (c) 1997 Aaron B. Brown.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Harvard University
 *      and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY HARVARD AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL HARVARD UNIVERSITY OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Driver for Pentium Pro counters.
 *
 * Written by Aaron Brown (abrown@eecs.harvard.edu)
 *
 * $Id: p6counter.h,v 1.3 1997/06/27 00:17:59 abrown Exp $
 */

#ifndef _I386_P6CNT_H_
#define _I386_P6CNT_H_

#ifndef i386			/* hopefully we're on a pentium pro */
#error Must be on a Pentium Pro!
#endif

#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>

/*********************************************************************
 ** Symbolic names for counter numbers (used in select_p6counter()) **
 *********************************************************************
 *
 * These correspond in order to the Pentium Pro counters. Add new counters at
 * the end. These agree with the mneumonics in the Pentium Pro Family
 * Developer's Manual, vol 3.
 *
 * Those events marked with a $ require a MESI unit field; those marked with
 * a @ require a self/any unit field. Those marked with a 0 are only supported
 * in counter 0; those marked with 1 are only supported in counter 1.
 */

/* Data cache unit */
#define P6_DATA_MEM_REFS	0x43	/* total memory refs */
#define P6_DCU_LINES_IN		0x45	/* all lines allocated in cache unit */
#define P6_DCU_M_LINES_IN	0x46	/* M lines allocated in cache unit */
#define P6_DCU_M_LINES_OUT	0x47	/* M lines evicted from cache */
#define P6_DCU_MISS_OUTSTANDING	0x48	/* #cycles a miss is outstanding */

/* Instruction fetch unit */
#define P6_IFU_IFETCH		0x80	/* instruction fetches */
#define P6_IFU_IFETCH_MISS	0x81	/* instruction fetch misses */
#define P6_ITLB_MISS		0x85	/* ITLB misses */
#define P6_IFU_MEM_STALL	0x86	/* number of cycles IFU is stalled */
#define P6_ILD_STALL		0x87	/* #stalls in instr length decode */

/* L2 Cache */
#define P6_L2_IFETCH		0x28	/* ($) l2 ifetches */
#define P6_L2_LD		0x29	/* ($) l2 data loads */
#define P6_L2_ST		0x2a	/* ($) l2 data stores */
#define P6_L2_LINES_IN		0x24	/* lines allocated in l2 */
#define P6_L2_LINES_OUT		0x26	/* lines removed from l2 */
#define P6_L2_M_LINES_INM	0x25	/* modified lines allocated in L2 */
#define P6_L2_M_LINES_OUTM	0x27	/* modified lines removed from L2 */
#define P6_L2_RQSTS		0x2e	/* ($) number of l2 requests */
#define P6_L2_ADS		0x21	/* number of l2 addr strobes */
#define P6_L2_DBUS_BUSY		0x22	/* number of data bus busy cycles */
#define P6_L2_DBUS_BUSY_RD	0x23	/* #bus cycles xferring l2->cpu */

/* External bus logic */
#define P6_BUS_DRDY_CLOCKS	0x62	/* (@) #clocks DRDY is asserted */
#define P6_BUS_LOCK_CLOCKS	0x63	/* (@) #clocks LOCK is asserted */
#define P6_BUS_REQ_OUTSTANDING	0x60	/* #bus requests outstanding */
#define P6_BUS_TRAN_BRD		0x65	/* (@) bus burst read txns */
#define P6_BUS_TRAN_RFO		0x66	/* (@) bus read for ownership txns */
#define P6_BUS_TRAN_WB		0x67	/* (@) bus writeback txns */
#define P6_BUS_TRAN_IFETCH	0x68	/* (@) bus instr fetch txns */
#define P6_BUS_TRAN_INVAL	0x69	/* (@) bus invalidate txns */
#define P6_BUS_TRAN_PWR		0x6a	/* (@) bus partial write txns */
#define P6_BUS_TRANS_P		0x6b	/* (@) bus partial txns */
#define P6_BUS_TRANS_IO		0x6c	/* (@) bus I/O txns */
#define P6_BUS_TRAN_DEF		0x6d	/* (@) bus deferred txns */
#define P6_BUS_TRAN_BURST	0x6e	/* (@) bus burst txns */
#define P6_BUS_TRAN_ANY		0x70	/* (@) total bus txns */
#define P6_BUS_TRAN_MEM		0x6f	/* (@) total memory txns */
#define P6_BUS_DATA_RCV		0x64	/* #busclocks CPU is receiving data */
#define P6_BUS_BNR_DRV		0x61	/* #busclocks CPU is driving BNR pin */
#define P6_BUS_HIT_DRV		0x7a	/* #busclocks CPU is driving HIT pin */
#define P6_BUS_HITM_DRV		0x7b	/* #busclocks CPU is driving HITM pin*/
#define P6_BUS_SNOOP_STALL	0x7e	/* #clkcycles bus is snoop-stalled */

/* FPU */
#define P6_FLOPS	       	0xc1	/* (0) number of FP ops retired */
#define	P6_FP_COMP_OPS		0x10	/* (0) computational FPOPS exec'd */
#define P6_FP_ASSIST		0x11	/* (1) FP excep's handled in ucode */
#define P6_MUL			0x12	/* (1) number of FP multiplies */
#define P6_DIV			0x13	/* (1) number of FP divides */
#define P6_CYCLES_DIV_BUSY	0x14	/* (0) number of cycles divider busy */

/* Memory ordering */
#define P6_LD_BLOCKS		0x03	/* number of store buffer blocks */
#define P6_SB_DRAINS		0x04	/* # of store buffer drain cycles */
#define P6_MISALING_MEM_REF	0x05	/* # misaligned data memory refs */

/* Instruction decoding and retirement */
#define P6_INST_RETIRED		0xc0	/* number of instrs retired */
#define P6_UOPS_RETIRED		0xc2	/* number of micro-ops retired */
#define P6_INST_DECODER		0xd0	/* number of instructions decoded */

/* Interrupts */
#define P6_HW_INT_RX		0xc8	/* number of hardware interrupts */
#define P6_CYCLES_INT_MASKED	0xc6	/* number of cycles hardints masked */
#define P6_CYCLES_INT_PENDING_AND_MASKED 0xc7 /* #cycles masked but pending */

/* Branches */
#define P6_BR_INST_RETIRED	0xc4	/* number of branch instrs retired */
#define P6_BR_MISS_PRED_RETIRED	0xc5	/* number of mispred'd brs retired */
#define P6_BR_TAKEN_RETIRED	0xc9	/* number of taken branches retired */
#define P6_BR_MISS_PRED_TAKEN_RET 0xca	/* #taken mispredictions br's retired*/
#define P6_BR_INST_DECODED    	0xe0	/* number of branch instrs decoded */
#define P6_BTB_MISSES		0xe2	/* # of branches that missed in BTB */
#define P6_BR_BOGUS		0xe4	/* number of bogus branches */
#define P6_BACLEARS		0xe6	/* # times BACLEAR is asserted */

/* Stalls */
#define P6_RESOURCE_STALLS	0xa2	/* # resource-related stall cycles */
#define P6_PARTIAL_RAT_STALLS	0xd2	/* # cycles/events for partial stalls*/

/* Segment register loads */
#define P6_SEGMENT_REG_LOADS	0x06	/* number of segment register loads */

/* Clocks */
#define P6_CPU_CLK_UNHALTED	0x79	/* #clocks CPU is not halted */

/* Unit field tags */
#define P6_UNIT_M		0x0800
#define P6_UNIT_E		0x0400
#define P6_UNIT_S		0x0200
#define P6_UNIT_I		0x0100
#define P6_UNIT_MESI		0x0f00

#define P6_UNIT_SELF		0x0000
#define P6_UNIT_ANY		0x2000

/****************************************************************************
 ** Flag bit definitions (used for the 'flag' field in select_p6counter()) **
 ****************************************************************************
 *
 * The driver accepts fully-formed counter specifications from user-level.
 * The following flags are mneumonics for the bits that get set in the
 * PerfEvtSel0 and PerfEvtSel1 MSR's
 *
 */
#define P6CNT_U  0x010000	/* Monitor user-level events */
#define P6CNT_K  0x020000	/* Monitor kernel-level events */
#define P6CNT_E	 0x040000	/* Edge detect: count state transitions */
#define P6CNT_PC 0x080000	/* Pin control: ?? */
#define P6CNT_IE 0x100000	/* Int enable: enable interrupt on overflow */
#define P6CNT_F  0x200000	/* Freeze counter (handled in software) */
#define P6CNT_EN 0x400000	/* enable counters (in PerfEvtSel0) */
#define P6CNT_IV 0x800000	/* Invert counter mask comparison result */

/***********************
 ** IOCTL definitions **
 ***********************/
#ifndef CIOCSETC
#define CIOCSETC _IOW('c', 1, unsigned int)  /* Set counter function */
#endif
#ifndef CIOCGETC
#define CIOCGETC _IOR('c', 2, unsigned int)  /* Get counter function */
#endif
#ifndef CIOCZERO
#define CIOCZERO _IO('c', 3)                 /* Zero counter */
#endif
#ifndef CIOCFRZ
#define CIOCFRZ _IO('c', 4)                  /* Freze counter value */
#endif
#ifndef CIOCUFRZ
#define CIOCUFRZ _IO('c', 5)                 /* Unfreze counter value */
#endif

/*****************************
 ** Miscellaneous constants **
 *****************************
 *
 * Number of Pentium Pro programable hardware counters. 
 */
#define NUM_P6HWC 2

/* Minor device number of master control device. */
#define P6TSC_DEV 0x4   /* Pentium cycle counter */
#define P6IDL_DEV 0x6   /* Idle loop counter */
#define P6CTL_DEV 0x7   /* Control device to freeze/zero all counters */
#define P6MAX_DEV P6CTL_DEV

/*******************************
 ** Macros to access counters **
 *******************************
 *
 *  Read 64 bit time stamp counter.  Put the high 32 bits in
 *  <*hi>, and the lower 32 bits in <*lo>.
 */
#define RDTSC(res_p) \
  __asm __volatile(						\
	 ".byte 0xf; .byte 0x31	# RDTSC instruction		\n\
         movl %%edx, %1		# High order 32 bits		\n\
         movl %%eax, %0		# Low order 32 bits"		\
	: "=g" (*(int *)(res_p)), "=g" (*(((int *)res_p)+1)) 	\
	: /* no input regs */					\
	: "eax", "edx")

/*
 *  Read 40 bit performance monitoring counter. Similar to above.
 */
#define RDPMC(cntr, res_p) \
  __asm __volatile(						\
	 "movl %2, %%ecx	# put counter number in		\n\
	 .byte 0xf; .byte 0x33	# RDPMC instruction		\n\
         movl %%edx, %1		# High order 32 bits		\n\
         movl %%eax, %0		# Low order 32 bits"		\
	: "=g" (*(int *)(res_p)), "=g" (*(((int *)res_p)+1)) 	\
	: "g" (cntr)						\
	: "ecx", "eax", "edx")

/*********************************************
 ** Functions to select and access counters **
 *********************************************/

static int __p6_cntr_valid[2] = {0, 0};
const char __p6_p6cnt_dev[] = "/dev/p6cnt";
const char __p6_p6cntctl_dev[] = "/dev/p6cntctl";

/* Guaranteed open */
static inline int
__p6_open(int dev, int opt)
{
	int fd;
	char path[sizeof(__p6_p6cnt_dev) + 14];

	if (dev == P6CTL_DEV)
	    strcpy(path, __p6_p6cntctl_dev);
	else
	    sprintf(path, "%s%d", __p6_p6cnt_dev, dev);

	fd = open(path, opt);
	if (fd < 0) {
		perror(path);
		exit(1);
	}
	return(fd);
}

/*
 * select_p6counter(): select what each counter will count 
 */
static void
select_p6counter(int cntrnum, u_int32_t flags)
{
	int fd;
	unsigned int sel = 0;

	if (cntrnum < 0 || cntrnum >= NUM_P6HWC) {
		fprintf(stderr, "Invalid counter number %d\n",cntrnum);
		exit(1);
	}

	if (!(flags & 0x030000))
		sel = P6CNT_K | P6CNT_U;
	sel |= (int) flags;

	fd = __p6_open(cntrnum, O_WRONLY);
	if (ioctl(fd, CIOCSETC, &sel) < 0) {
		fprintf(stderr, "counter %d: ", cntrnum);
		perror("CIOCSETC");
		exit(1);
	}
	close(fd);
	__p6_cntr_valid[cntrnum] = 1;
}	

/*
 * read_p6conters(): read the values of the counters into a (pre-allocated)
 * 		     array.
 */
static inline void 
read_p6counters(u_int64_t *vals)
{
	int fd;
	char cntval[64];
	int cntrnum;
	int n;
	char *s2;
	unsigned int val;

	for (cntrnum = 0; cntrnum < NUM_P6HWC; cntrnum++) {
		if (!__p6_cntr_valid[cntrnum]) {
			vals[cntrnum] = 0;
			continue;
		}

		RDPMC(cntrnum, &vals[cntrnum]);
	}
}

#endif _I386_P6CNT_H_
