#!/usr/bin/make

CC=gcc
CFLAGS=-mno-fp-regs -Wall -Wstrict-prototypes -O2 -pipe	-ffixed-8	\
	-I../../../../include -I../../linux/include -I../include	\
	-I$(HOME)/Cplant/top/compute/OS/linux/include

all:	kmemcpy.o memcpy

kmemcpy.o:	kmemcpy.c
memcpy:		memcpy.c
