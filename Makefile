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

######################################
##                                  ##
## Top-level Makefile for HBench-OS ##
##                                  ##
######################################
##
## $Id: Makefile,v 1.2 1997/06/27 00:32:32 abrown Exp $
##

##
## TARGETS
##
#
# build		(default) build binaries for the current architecture,
#               including cyclecounter-enabled binaries, if supported
# clobber	remove all binaries and results for the current architecture
# clobberall	remove all binaries, and entire contents of Results directory
# setup		interactive setup for a run on the current machine
# run		run the benchmarks using the run file corresponding to
#               the hostname
# summary	produce a summary version of all stored results

SHELL    = /bin/bash
PLATFORM = $(shell ../scripts/config.guess)
ARCH     = $(shell echo $(PLATFORM) | sed 's/-.*-.*$$//')
HOSTNAME = `hostname | sed 's/\..*$$//'`

build:
	@cd src && $(MAKE)
# XXX should test if compiler is gcc in src/Makefile
	@if [ $(ARCH) = i386 -o $(ARCH) = i586 ]; then (cd src && $(MAKE) cyclecounter); fi

clean:
	@echo "clean is not a valid target; consider using clobber (but be"
	@echo "aware that it will remove both binaries AND results!)"

clobber:
	cd src && $(MAKE) clean
	cd Results && $(MAKE) clean

clobberall:
	cd src && $(MAKE) cleanall
	cd Results && $(MAKE) cleanall

setup: build
	@$(SHELL) ./scripts/interactive-setup

run:
	@echo $(SHELL) ./scripts/maindriver ./conf/$(HOSTNAME).run
	@$(SHELL) ./scripts/maindriver ./conf/$(HOSTNAME).run

summary:
	cd Results && $(MAKE) summary
