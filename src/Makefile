#
# Copyright (c) 1997 The President and Fellows of Harvard College.
# All rights reserved.
# Copyright (c) 1997 Aaron B. Brown.
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program, in the file COPYING in this distribution;
#   if not, write to the Free Software Foundation, Inc., 675 Mass Ave,
#   Cambridge, MA 02139, USA.
#
# Results obtained from this benchmark may be published only under the
# name "HBench-OS".
#
# $Id: Makefile,v 1.22 1997/06/27 14:50:12 abrown Exp $

# Make targets:
#
# hbench	[default] builds the benchmark suite for the current os/arch
# cyclecounter	build the suite with cyclecounter support
# eventcountersP5 build with Pentium event counter support
# eventcountersP6 build with Pentium Pro event counter support
# clean		removes binaries for the current platform
# cleanall	removes binaries for *all* platforms
# depend	build (REQUIRED!) dependecy rules
#
# Note that make clean actually removes the binaries (since they're
# never really "installed" anywhere).
#
# This is largely self configuring.  Most stuff is pretty portable.  You
# may have problems with the mhz program.  It needs a C compiler smart
# enough to use "register" when running in unoptimized mode.  gcc does
# this.  cc may not.  Look at the output of mhz and see if it is close
# (within a few percent).  If not, try a different compiler and send me
# mail with the info.
#
# Note that mhz will not work on an unpipelined, non-RISC CPU like the i386.
#
# OS config.  If your OS is not special-cased below, and the defaults don't
# work, then you'll need to add a case below setting up the appropriate
# compiler and flags. If you do this, send me mail with the diffs to this
# makefile.
#
# If you don't have gcc, try make CC=cc and see if that works.

SHELL=/bin/sh

CC = musl-gcc
PLATFORM=`$(SHELL) ../scripts/config.guess`
ARCH=`$(SHELL) ../scripts/config.guess | sed 's/-.*-.*$$//'`
OS=`$(SHELL) ../scripts/config.guess | sed 's/^.*-.*-//'`
OSROOT=`$(SHELL) ../scripts/config.guess | sed 's/^.*-.*-//' | sed 's/[0-9].*$$//'`

all:
	@$(MAKE) BINDIR=../bin/$(OS)-$(ARCH) $(OSROOT)

cyclecounter:
	@$(MAKE) COUNTERS=-DCYCLE_COUNTER BINDIR=../bin/$(OS)-$(ARCH)-c $(OSROOT)

eventcountersP5:
	@$(MAKE) COUNTERS=-DEVENT_COUNTERS=5 BINDIR=../bin/$(OS)-$(ARCH)-ec $(OSROOT)

eventcountersP6:
	@$(MAKE) COUNTERS=-DEVENT_COUNTERS=6 BINDIR=../bin/$(OS)-$(ARCH)-ec $(OSROOT)

CFLAGS= -static -DNO_PORTMAPPER -O $(SYS5) $(COUNTERS)

COMPILE=$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS)

#####################################
##                                 ##
## PER-OS CONFIGURATION SECTION    ##
##                                 ##
## Make sure your operating system ##
## has a target here. Each target  ##
## should setup any needed flags,  ##
## then invoke the default target  ##
## (make binaries).                ##
##                                 ##
#####################################

# The following don't need special handling.
freebsd netbsd openbsd sunos linux:
	@$(MAKE) binaries

bsdi:
	@$(MAKE) LDLIBS="-lrpc" binaries

solaris:
	@$(MAKE) CC=cc LDLIBS="-lnsl -lsocket" SYS5=-DSYS5 binaries

# HPUX needs gcc; their C compiler screws up mhz.c.
hpux:
	@$(MAKE) CC="$(CC)" CFLAGS="$(CFLAGS) -Dvalloc=malloc -DNO_RUSAGE" binaries

# Really specific to the alpha, not osf.
osf:
	@$(MAKE) CC=cc binaries

irix:
	@$(MAKE) CC=cc CFLAGS="$(CFLAGS) -32" binaries

aix:
	@$(MAKE) CC=cc CFLAGS="$(CFLAGS) -Dvalloc=malloc" binaries


#########################################
##                                     ##
## END OF PER-OS CONFIGURATION SECTION ##
##                                     ##
## You should not need to change       ##
## anything below this box.            ##
##                                     ##
#########################################

setupdirs:
	@echo Building for $(OS)-$(ARCH) into $(BINDIR)
	@if [ ! -d $(BINDIR) ]; then mkdir -p $(BINDIR); fi

SRCS=	bench.h bw_bzero.c bw_file_rd.c bw_mem_cp.c bw_mem_rd.c bw_mem_wr.c \
	bw_mmap_rd.c bw_pipe.c bw_tcp.c common.c counter-common.c hello.c \
	lat_connect.c lat_ctx.c lat_ctx2.c lat_fs.c lat_fslayer.c \
	lat_mem_rd.c lat_mmap.c lat_pipe.c lat_proc.c lat_rpc.c \
	lat_sig.c lat_syscall.c lay_tcp.c lat_udp.c lib_tcp.c lib_udp.c \
	memsize.c mhz.c timing.c utils.c lmdd.c lat_pagefault.c

#EXES=	$(BINDIR)/lat_syscall $(BINDIR)/hello $(BINDIR)/hello-s \

NAMES=	bw_bzero bw_file_rd \
	bw_mem_cp bw_mem_rd bw_mem_wr \
	bw_mmap_rd \
	bw_pipe bw_tcp \
	lat_connect \
	lat_ctx lat_ctx2 \
	lat_fs lat_fslayer \
	lat_mem_rd \
	lat_mmap \
	lat_pipe \
	lat_proc \
	lat_sig \
	lat_syscall \
	lat_tcp \
	lat_udp \
	memsize hello hello-s \
	mhz mhz-counter \
#	lmdd \
#	lat_pagefault \

EXES= $(addprefix $(BINDIR)/, $(addsuffix $(EXT),$(NAMES)))


binaries: setupdirs $(EXES)

clean:
	@echo rm -rf ../bin/$(OS)-$(ARCH)*
	@/bin/rm -rf ../bin/$(OS)-$(ARCH)*

cleanall:
	@echo rm -rf ../bin/[a-z]*
	@rm -rf ../bin/[a-z]*

depend:
	@../scripts/depend

###
### SPECIAL-CASE PROGRAMS
###

# Build static hello, if possible
$(BINDIR)/hello-s$(EXT): hello.c
	@echo Generating statically-linked hello...
	$(COMPILE) -static -o $@ hello.c $(LDLIBS)
# Move this line above the "fi" (and backslash) to try a gcc-version of static
#	else $(COMPILE) -static -o $(BINDIR)/hello-s hello.c $(LDLIBS);


# Build dynamic hello
$(BINDIR)/hello$(EXT): hello.c
	@echo Generating dynamically-linked hello...
	@if [ $(OSROOT) = bsdi ]; \
	then shlicc -O -o $(BINDIR)/hello hello.c $(LDLIBS); \
	else $(COMPILE) -o $@ hello.c $(LDLIBS); \
	fi

# No optimization for these.
$(BINDIR)/mhz$(EXT): mhz.c common.c bench.h timing.c utils.c counter-common.c
	@echo Compiling mhz...
	$(CC) $(SYS5) -o $@ mhz.c

$(BINDIR)/mhz-counter$(EXT): mhz.c common.c bench.h timing.c utils.c counter-common.c
	@if [ X$(COUNTERS)X != XX ]; \
	then $(CC) $(COUNTERS) $(SYS5) -o $@ mhz.c; \
	fi

$(BINDIR)/lat_ctx2$(EXT): lat_ctx2.c
	@echo $(COMPILE)-o $(BINDIR)/lat_ctx2 lat_ctx2.c $(LDLIBS)
	@if [ $(OSROOT) = irix ];\
	then	$(CC) $(SYS5) -32 -O1 -o $(BINDIR)/lat_ctx2 lat_ctx2.c;\
	else	$(COMPILE) -o $@ lat_ctx2.c $(LDLIBS);\
	fi

# XXX not currently supported
#$(BINDIR)/lat_pagefault:  lat_pagefault.c timing.c bench.h
#	@if [ $(OSROOT) = linux -o $(OSROOT) = bsd ];\
#	then    cp /bin/true $(BINDIR)/lat_pagefault;\
#	else    $(COMPILE) -o $(BINDIR)/lat_pagefault lat_pagefault.c $(LDLIBS);\
#	fi

# Do not remove the next line, $(MAKE) depend needs it
# MAKEDEPEND follows
$(BINDIR)/bw_bzero$(EXT):  bw_bzero.c common.c bench.h counter-common.c timing.c  utils.c
	$(COMPILE) -o $@ bw_bzero.c $(LDLIBS)

$(BINDIR)/bw_file_rd$(EXT):  bw_file_rd.c common.c bench.h counter-common.c timing.c  utils.c
	$(COMPILE) -o $@ bw_file_rd.c $(LDLIBS)

$(BINDIR)/bw_mem_cp$(EXT):  bw_mem_cp.c common.c bench.h counter-common.c timing.c  utils.c
	$(COMPILE) -o $@ bw_mem_cp.c $(LDLIBS)

$(BINDIR)/bw_mem_rd$(EXT):  bw_mem_rd.c common.c bench.h counter-common.c timing.c  utils.c
	$(COMPILE) -o $@ bw_mem_rd.c $(LDLIBS)

$(BINDIR)/bw_mem_wr$(EXT):  bw_mem_wr.c common.c bench.h counter-common.c timing.c  utils.c
	$(COMPILE) -o $@ bw_mem_wr.c $(LDLIBS)

$(BINDIR)/bw_mmap_rd$(EXT):  bw_mmap_rd.c common.c bench.h counter-common.c timing.c  utils.c
	$(COMPILE) -o $@ bw_mmap_rd.c $(LDLIBS)

$(BINDIR)/bw_pipe$(EXT):  bw_pipe.c common.c bench.h counter-common.c timing.c  utils.c
	$(COMPILE) -o $@ bw_pipe.c $(LDLIBS)

$(BINDIR)/bw_tcp$(EXT):  bw_tcp.c common.c bench.h counter-common.c timing.c utils.c  lib_tcp.c
	$(COMPILE) -o $@ bw_tcp.c $(LDLIBS)

$(BINDIR)/common:  common.c bench.h counter-common.c timing.c utils.c
	$(COMPILE) -o $(BINDIR)/common common.c $(LDLIBS)

$(BINDIR)/counter-common:  counter-common.c
	$(COMPILE) -o $(BINDIR)/counter-common counter-common.c $(LDLIBS)

$(BINDIR)/lat_connect$(EXT):  lat_connect.c common.c bench.h counter-common.c  timing.c utils.c lib_tcp.c
	$(COMPILE) -o $@ lat_connect.c $(LDLIBS)

$(BINDIR)/lat_ctx$(EXT):  lat_ctx.c common.c bench.h counter-common.c timing.c  utils.c
	$(COMPILE) -o $@ lat_ctx.c $(LDLIBS)

$(BINDIR)/lat_fs$(EXT):  lat_fs.c common.c bench.h counter-common.c timing.c utils.c
	$(COMPILE) -o $@ lat_fs.c $(LDLIBS)

$(BINDIR)/lat_fslayer$(EXT):  lat_fslayer.c common.c bench.h counter-common.c  timing.c utils.c
	$(COMPILE) -o $@ lat_fslayer.c $(LDLIBS)

$(BINDIR)/lat_mem_rd$(EXT):  lat_mem_rd.c common.c bench.h counter-common.c timing.c  utils.c
	$(COMPILE) -o $@ lat_mem_rd.c $(LDLIBS)

$(BINDIR)/lat_mmap$(EXT):  lat_mmap.c common.c bench.h counter-common.c timing.c  utils.c
	$(COMPILE) -o $@ lat_mmap.c $(LDLIBS)

$(BINDIR)/lat_pipe$(EXT):  lat_pipe.c common.c bench.h counter-common.c timing.c  utils.c
	$(COMPILE) -o $@ lat_pipe.c $(LDLIBS)

$(BINDIR)/lat_proc$(EXT):  lat_proc.c common.c bench.h counter-common.c timing.c  utils.c
	$(COMPILE) -o $@ lat_proc.c $(LDLIBS)

$(BINDIR)/lat_rpc$(EXT):  lat_rpc.c common.c bench.h counter-common.c timing.c  utils.c
	$(COMPILE) -o $@ lat_rpc.c $(LDLIBS)

$(BINDIR)/lat_sig$(EXT):  lat_sig.c common.c bench.h counter-common.c timing.c  utils.c
	$(COMPILE) -o $@ lat_sig.c $(LDLIBS)

$(BINDIR)/lat_syscall$(EXT):  lat_syscall.c common.c bench.h counter-common.c  timing.c utils.c
	$(COMPILE) -o $@ lat_syscall.c $(LDLIBS)

$(BINDIR)/lat_tcp$(EXT):  lat_tcp.c common.c bench.h counter-common.c timing.c  utils.c lib_tcp.c
	$(COMPILE) -o $@ lat_tcp.c $(LDLIBS)

$(BINDIR)/lat_udp$(EXT):  lat_udp.c common.c bench.h counter-common.c timing.c  utils.c lib_udp.c
	$(COMPILE) -o $@ lat_udp.c $(LDLIBS)

$(BINDIR)/lib_tcp$(EXT):  lib_tcp.c bench.h
	$(COMPILE) -o $@ lib_tcp.c $(LDLIBS)

$(BINDIR)/lib_udp$(EXT):  lib_udp.c bench.h
	$(COMPILE) -o $@ lib_udp.c $(LDLIBS)

$(BINDIR)/memsize$(EXT):  memsize.c common.c bench.h counter-common.c timing.c  utils.c
	$(COMPILE) -o $@ memsize.c $(LDLIBS)

$(BINDIR)/timing$(EXT):  timing.c bench.h
	$(COMPILE) -o $@ timing.c $(LDLIBS)

$(BINDIR)/utils$(EXT):  utils.c bench.h timing.c
	$(COMPILE) -o $@ utils.c $(LDLIBS)
