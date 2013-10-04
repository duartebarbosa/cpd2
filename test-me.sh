#!/bin/bash
# Bash script for running and compare project tests

# we will have to change these (and the paths) once the project assignment comes to light
PROCESSES=( 1 2 4 8 )
THREADS=( 1 2 4 8 )
FILES=( ex5-1d ex10-2d ex1000-50d ex1M-100d ex100k-200-3 ex100k-200-4-mod )

serial(){
	make clean docs_serial
	sync

	for((i=0; i < 6; i++)) do
		echo "_________________________________________"
		echo "input: "${FILES[i]}
		time ./docs-serial sampleDocInstances/in/${FILES[i]}.in
		cmp sampleDocInstances/in/${FILES[i]}.out sampleDocInstances/out/${FILES[i]}.out
	done
}

omp(){
	make clean docs_omp
	sync

	for((j=0; j < 4; j++)) do
		export OMP_NUM_THREADS=${THREADS[j]}
		echo "threads: "${THREADS[j]}
		for((i=0; i < 6; i++)) do
			echo "_________________________________________"
			echo "input: "${FILES[i]}
			time ./docs-omp sampleDocInstances/in/${FILES[i]}.in
			cmp sampleDocInstances/in/${FILES[i]}.out sampleDocInstances/out/${FILES[i]}.out
		done
	done
}

mpi(){
	make clean docs_mpi
	sync

	for((j=0; j < 4; j++)) do
		echo "processes: "${PROCESSES[j]}
		for((i=0; i < 6; i++)) do
			echo "_________________________________________"
			echo "input: "${FILES[i]}
			time /usr/lib64/openmpi/bin/mpirun -np ${PROCESSES[j]} ./docs-mpi sampleDocInstances/in/${FILES[i]}.in
			cmp sampleDocInstances/in/${FILES[i]}.out sampleDocInstances/out/${FILES[i]}.out
		done
	done
}

omp_mpi(){

	make clean docs_mpi_omp
	sync

	for((k=0; k < 4; k++)) do
		echo "processes: "${PROCESSES[k]}
		for((j=0; j < 4; j++)) do
			export OMP_NUM_THREADS=${THREADS[j]}
			echo "threads: "${THREADS[j]}
			for((i=0; i < 6; i++)) do
				echo "_________________________________________"
				echo "input: "${FILES[i]}
				time /usr/lib64/openmpi/bin/mpirun -np ${PROCESSES[k]} ./docs-mpi-omp sampleDocInstances/in/${FILES[i]}.in
				cmp sampleDocInstances/in/${FILES[i]}.out sampleDocInstances/out/${FILES[i]}.out
			done
		done
	done
	exit
}

start(){
	make mrproper
	clear
	mkdir log

	(serial) &> log/serial.log
	(omp) &> log/omp.log
	(mpi) &> log/mpi.log
	(omp_mpi) &> log/mpi-omp.log

	make clean &> /dev/null
	exit
}

start

