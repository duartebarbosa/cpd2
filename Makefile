GROUP = xx
CC = gcc
DEBUGFLAGS = -ansi -Wall -pedantic -g
GOODFLAGS = -O3 -march=native
TIMER = -DGETTIME=0

all: clean build

setup:
	wget ...

deploy: mrproper zip

build: serial omp mpi mpi_omp

test:
	bash test-me.sh

serial: wolves-squirrels-serial.c
	$(CC) $(GOODFLAGS) $(TIMER) -fopenmp wolves-squirrels-serial.c -o wolves-squirrels-serial

omp: wolves-squirrels-omp.c
	$(CC) $(GOODFLAGS) $(TIMER) -fopenmp wolves-squirrels-omp.c -o wolves-squirrels-omp

mpi: wolves-squirrels-mpi.c
	/usr/lib64/openmpi/bin/mpicc $(GOODFLAGS) $(TIMER) -g wolves-squirrels-mpi.c -o wolves-squirrels-mpi

mpi_omp: wolves-squirrels-mpi-omp.c
	/usr/lib64/openmpi/bin/mpicc $(GOODFLAGS) $(TIMER) -fopenmp -g wolves-squirrels-mpi-omp.c -o wolves-squirrels-mpi-omp

backup:
	@-tar -czf wolves-squirrels.tgz Makefile *.c report.pdf

clean:
	rm -f wolves-squirrels-serial wolves-squirrels-omp wolves-squirrels-mpi wolves-squirrels-mpi *.o

mrproper: clean
	rm -rf log/ *.~ *~ instances/out/

zip_omp:
	@-tar -czf g$(GROUP)omp.zip Makefile wolves-squirrels-serial.c wolves-squirrels-omp.c report.pdf

zip_mpi:
	@-tar -czf g$(GROUP)mpi.zip Makefile wolves-squirrels-serial.c wolves-squirrels-mpi.c report.pdf

.PHONY: clean zip
