#!/bin/bash
# Bash script for running and compare project tests

WOLF_BREEDING=3
WOLF_STARVATION=4
SQUIRREL_BREEDING=4
GENERATIONS=15

CONFIG=""

PROCESSES=( 1 2 4 8 )
THREADS=( 1 2 4 8 )
FILES=( ex3 world_10 world_100 world_1000 world_10000 )

serial(){
	mkdir -p instances/out/serial
	make clean serial
	sync

	for((i=0; i < 5; i++)) do
		echo "_________________________________________"
		echo -n "${FILES[i]}: "
		/usr/bin/time ./wolves-squirrels-serial instances/${FILES[i]}.in $CONFIG > instances/out/serial/${FILES[i]}.out
		cmp instances/out/serial/${FILES[i]}.out instances/${FILES[i]}.out
	done
}

omp(){
	mkdir -p instances/out/omp
	make clean omp
	sync

	for((j=0; j < 4; j++)) do
		export OMP_NUM_THREADS=${THREADS[j]}
		echo "threads: "${THREADS[j]}
		for((i=0; i < 5; i++)) do
			echo "_________________________________________"
			echo -n "${FILES[i]}: "
			/usr/bin/time ./wolves-squirrels-omp instances/${FILES[i]}.in $CONFIG > instances/out/omp/${FILES[i]}.out
			cmp instances/out/omp/${FILES[i]}.out instances/${FILES[i]}.out
		done
	done
}

mpi(){
	make clean mpi
	sync

	for((j=0; j < 4; j++)) do
		echo "processes: "${PROCESSES[j]}
		for((i=0; i < 5; i++)) do
			echo "_________________________________________"
			echo -n "${FILES[i]}: "
			time /usr/lib64/openmpi/bin/mpirun -np ${PROCESSES[j]} ./wolves-squirrels-mpi sampleDocInstances/in/${FILES[i]}.in
			cmp sampleDocInstances/in/${FILES[i]}.out sampleDocInstances/out/${FILES[i]}.out
		done
	done
}

omp_mpi(){

	make clean mpi_omp
	sync

	for((k=0; k < 4; k++)) do
		echo "processes: "${PROCESSES[k]}
		for((j=0; j < 4; j++)) do
			export OMP_NUM_THREADS=${THREADS[j]}
			echo "threads: "${THREADS[j]}
			for((i=0; i < 5; i++)) do
				echo "_________________________________________"
				echo -n "${FILES[i]}: "
				time /usr/lib64/openmpi/bin/mpirun -np ${PROCESSES[k]} ./wolves-squirrels-mpi-omp sampleDocInstances/in/${FILES[i]}.in
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
	CONFIG="$WOLF_BREEDING $WOLF_STARVATION $SQUIRREL_BREEDING $GENERATIONS"
	export TIME="%E"

	echo "testing..."
	(serial) &> log/serial.log
	(omp) &> log/omp.log
	(mpi) &> log/mpi.log
	(omp_mpi) &> log/mpi-omp.log

	make clean &> /dev/null
	exit
}

start

