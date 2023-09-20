# Makefile for power control stuff.   

CC = gcc

help:
	@echo "make options:"
	@echo "    power - image for use with building 880 machine."
	@echo "    power.puma - image for use with puma trailer machine."
	@echo "    all - builds all images."
	@echo " "
	@echo "Read the comments at the top of power.c for more information."
	@echo " "

all: power power.puma

power: power.c
	$(CC) -o power -DCPLANT_1 power.c

power.puma: power.c
	$(CC) -o power.puma -DPUMA_TRAILER  power.c
