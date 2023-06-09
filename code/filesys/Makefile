# NOTE: this is a GNU Makefile.  You must use GNU Make; other `make`
# implementations may not work.
#
# Makefile for the file system assignment.
# Defines set up assuming multiprogramming and virtual memory done first.
# If not, use the “bare bones” defines below.
#
# Copyright (c) 1992      The Regents of the University of California.
#               2016-2021 Docentes de la Universidad Nacional de Rosario.
# All rights reserved.  See `copyright.h` for copyright notice and
# limitation of liability and disclaimer of warranty provisions.

DEFINES      = -DUSER_PROGRAM -DVMEM -DFILESYS_NEEDED -DFILESYS     \
                          -DUSE_TLB -DDFS_TICKS_FIX -DDEMAND_LOADING -DSWAP -DPRPOLICY_LRU 
INCLUDE_DIRS = -I.. -I../bin -I../vm -I../userprog -I../threads -I../machine
HDR_FILES    = $(THREAD_HDR) $(USERPROG_HDR) $(VMEM_HDR) $(FILESYS_HDR)
SRC_FILES    = $(THREAD_SRC) $(USERPROG_SRC) $(VMEM_SRC) $(FILESYS_SRC)
OBJ_FILES    = $(THREAD_OBJ) $(USERPROG_OBJ) $(VMEM_OBJ) $(FILESYS_OBJ)

# Bare bones version.
#DEFINES      = -DTHREADS -DFILESYS_NEEDED -DFILESYS
#INCLUDE_DIRS = -I.. -I../threads -I../machine
#HDR_FILES    = $(THREAD_HDR) $(FILESYS_HDR)
#SRC_FILES    = $(THREAD_SRC) $(FILESYS_SRC)
#OBJ_FILES    = $(THREAD_OBJ) $(FILESYS_OBJ)

include ../Makefile.common
include ../Makefile.env
-include Makefile.depends

clean:
	@echo ":: Cleaning $$(tput bold)$(notdir $(CURDIR))$$(tput sgr0)"
	@rm "../DISK" || true

PROGRAMS = echo filetest halt matmult shell sort tiny_shell touch cat rm cp smol_test libtest memory_test_a \
		   memory_test_b memory_test_c mkdir ls write

all: ../DISK

../DISK:
	@./nachos -f
	@$(foreach var, $(PROGRAMS), ./nachos -cp ../userland/$(var) $(var);)
	mv ./DISK ../DISK