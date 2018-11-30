# MPI

MPICC=/usr/bin/mpicc
#MPICC=/usr/lib64/openmpi/bin/mpicc

mpitest: invaders.c
	${MPICC} invaders.c -o invaders -lm -lncurses
