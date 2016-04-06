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
# $Id: Makefile,v 1.3 1997/06/27 00:37:47 abrown Exp $

##
## TARGETS
##
#
# clean		remove all results for the current architecture
# cleanall	remove all results
# summary	summary comparison of the various stored results
#
# latencygraphs	produce gnuplot-input files for latency graphs for each machine
# summaries	produce summaries for each result directory, stored in
#		a "summary" file in each machine's data directory
# analyses	produce analyses of each machine's data, stored in an
#		"analysis" file in each machine's data directory
#
# NOTE: BOTH CLEAN AND CLEANALL ARE VERY DANGEROUS, AND CAN DESTROY
#       RESULTS! USE WITH CARE!

SHELL=/bin/sh
ARCH=`$(SHELL) ../scripts/config.guess | sed 's/-.*-.*$$//'`
OS=`$(SHELL) ../scripts/config.guess | sed 's/^.*-.*-//'`

default:
	@echo "Select one of the following targets:"
	@echo "clean            remove all results for the current architecture"
	@echo "cleanall         remove all results"
	@echo "summary          summary comparison of the various stored results"
	@echo
	@echo "latencygraphs    produce gnuplot-input files for latency graphs for each machine"
	@echo "summaries        produce summaries for each result directory, stored in"
	@echo "                 a \"summary\" file in each machine's data directory"
	@echo "analyses         produce analyses of each machine's data, stored in"
	@echo "                 an \"analysis\" file in each machine's data directory"
	@echo
	@echo "NOTE: BOTH CLEAN AND CLEANALL ARE VERY DANGEROUS, AND CAN DESTROY"
	@echo "RESULTS! USE WITH CARE!"

clean:
	@echo rm -rf $(OS)-$(ARCH)
	@/bin/rm -rf $(OS)-$(ARCH)

cleanall:
	@echo rm -rf [a-z]*-*
	@rm -rf [a-z]*-*

summary:
	@echo "The summary feature is not implemented in this version of HBench-OS."
	@echo
	@echo "Each machine's results directory under hbench-OS/Results contains a file"
	@echo "named 'summary', which contains the results summary for that machine."
	@echo "You will also find a file named 'analysis' that contains some"
	@echo "rudimentary automatic analysis of the data in the same place."
	@echo "Finally, by running hbench-OS/scripts/gen-latgraph, you can generate"
	@echo "a graph of the memory hierarchy latency for a given machine."

summaries:
	$(SHELL) ../scripts/create-all-summaries

analyses:
	$(SHELL) ../scripts/create-all-analyses

latencygraphs:
	$(SHELL) ../scripts/create-all-latgraphs
