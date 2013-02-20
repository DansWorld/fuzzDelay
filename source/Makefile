INCLUDE = -I../include 
LIB = -L../lib -lportsf -lportaudio -lfxdelay
CC = gcc

PORTSF = ../lib/libportsf.a
FXDELAY = ../lib/libfxdelay.a

fuzzDelay: fuzzDelay.c $(PORTSF) $(FXDELAY)
	$(CC) -o fuzzDelay fuzzDelay.c $(INCLUDE) $(LIB)

$(PORTSF): 
	$(CC) -o portsf.o portsf.c -c $(INCLUDE)
	$(CC) -o ieee80.o ieee80.c -c $(INCLUDE)
	ar -r $(PORTSF) portsf.o ieee80.o
	rm ieee80.o
	rm portsf.o

$(FXDELAY): fxdelay.c ../include/fxdelay.h
	$(CC) -o fxdelay.o fxdelay.c -c $(INCLUDE) -c
	ar -r $(FXDELAY) fxdelay.o
	rm fxdelay.o

