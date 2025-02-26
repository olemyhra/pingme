CC = gcc
FLAGS = -Wall -Werror -O2 -g3 -std=c17 -pedantic

pingme: pingme.c
	$(CC) $(FLAGS) $^ -o $@
	
clean:
	rm -f pingme
