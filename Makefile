# Space Invaders Makefile

invaders: TestInvaders.c invaderstructs.h
	gcc -o invaders TestInvaders.c -lm -lncurses
