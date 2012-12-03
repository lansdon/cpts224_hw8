
CC=gcc
TARGETS=libcma.so ma_group clean #dist
OPTIONS=-g 
all: $(TARGETS)

ma_group: ma ma2

libcma.so: cma.o
	$(CC) $(OPTIONS) -shared cma.o -o libcma.so
ma: ma.o cma.o 
	$(CC) $(OPTIONS) ma.o -o ma -L. -lcma
ma2: ma2.o cma.o 
	$(CC) $(OPTIONS) ma2.o -o ma2 -L. -lcma
cma.o: cma.c cma.h
	$(CC) $(OPTIONS) -c cma.c -o cma.o
ma.o: ma.c cma.h cma.c 
	$(CC) $(OPTIONS) -c ma.c -o ma.o
ma2.o: ma2.c cma.h cma.c 
	$(CC) $(OPTIONS) -c ma2.c -o ma2.o
clean:
	-rm *.o
#dist:
#	-tar -cf lpage_hw6 *.c *.h make*
