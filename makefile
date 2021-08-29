# Paths
VPATH = src

# Coolprop static library
CPlib = -ldl -L./lib/coolprop/static -I./lib/coolprop/include -I./lib/coolprop/externals/fmtlib/ -lCoolProp

# Compiler
CC = g++ -Wall -O3

laine : laine.o text.o node.o polish.o matrix.o solver.o reduce.o
	$(CC) $^ -o $@ #$(CPlib)

# Objects
text.o : text.cc text.hpp
	$(CC) -c $< -o $@ 

node.o : node.cc node.hpp
	$(CC) -c $< -o $@ #$(CPlib)

polish.o : polish.cc polish.hpp 
	$(CC) -c $< -o $@ 

matrix.o : matrix.cc matrix.hpp
	$(CC) -c $< -o $@

solver.o : solver.cc solver.hpp
	$(CC) -c $< -o $@ 

reduce.o : reduce.cc reduce.hpp
	$(CC) -c $< -o $@ 

laine.o : laine.cc
	$(CC) -c $< -o $@

#demo : wasm.cc $(H_FILES)
#	emcc --bind -O3 ./src/wasm.cc -I./include -o ./laine.js

# Utilities
.PHONY: clean
clean :
	rm laine *.o
