SRC=/home/raouf/Documents/Projects/Parallel_Programing/Development/codes/modular/src
BUILD=/home/raouf/Documents/Projects/Parallel_Programing/Development/codes/modular/build

output: threads.o funcs.o main.o
	mpicc main.o threads.o funcs.o -o output -lpthread -lm

threads.o: $(SRC)/threads.c $(SRC)/header.h
	mpicc -g -c $(SRC)/threads.c -o threads.o -lpthread -lm

funcs.o: $(SRC)/funcs.c $(SRC)/header.h
	mpicc -g -c $(SRC)/funcs.c -o funcs.o -lpthread -lm

main.o: $(SRC)/main.c
	mpicc -g -c $(SRC)/main.c -o main.o -lpthread -lm

run: 
	mpirun -np 4 -oversubscribe output 2 2