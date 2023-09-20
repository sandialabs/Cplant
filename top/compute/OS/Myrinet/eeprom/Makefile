include ../../../../Makefile-common

CC          = gcc -g
CFLAGS      = -Wall 
GCC_LIB_DIR = /usr/lib
INC_DIR     = -I../include -I/usr/include
LIB_DIR     = -L../lib -L$(GCC_LIB_DIR)
LIBS        = -lLanaiDevice 

INSTALLDIR     = $(sbindir)
INSTALLTARGETS = eeprom

all:   eeprom

eeprom: eeprom.c
	$(CC) $(CFLAGS) $(INC_DIR) -o eeprom $^ $(LIB_DIR) $(LIBS)
	@echo ---------------------------------------------------------------

clean:
	@\rm -f *.o eeprom core

install uninstall cp: $(INSTALLTARGETS)
	@$($@)
