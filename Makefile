#MPI
#MPICC=/usr/bin/mpic++
MPICC=/usr/lib64/openmpi/bin/mpic++

FILE=invaders_mpi.c
#FILE=invaders.c

NUMPROC=2
#MPIRUN=/usr/bin/mpirun
MPIRUN=/usr/lib64/openmpi/bin/mpirun


invaders: ${FILE} invaderstructs.h
	${MPICC} ${FILE} -g -o invaders -lm -lncurses -I/tmp/NCURSES/include/ncurses -I/tmp/NCURSES/include -L/tmp/NCURSES/lib

run:
	${MPIRUN} -np ${NUMPROC} ./invaders

c:
	rm invaders
