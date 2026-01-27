CC = gcc

LIB = -lm `sdl2-config --libs` -fopenmp -lSDL2_ttf

SRC = main.c utils.c

NAME = fluid

all:
	$(CC) $(SRC) $(LIB) -o $(NAME)