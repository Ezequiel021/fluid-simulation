CC = gcc

LIB = -lm `sdl2-config --libs`

SRC = main.c utils.c

NAME = fluid

all:
	$(CC) $(SRC) $(LIB) -o $(NAME)