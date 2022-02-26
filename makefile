# Paths
VPATH = src

# Coolprop static library
CPlib = -ldl -L./lib/coolprop/static -I./lib/coolprop/include -I./lib/coolprop/externals/fmtlib/ -I./lib/coolprop/ -lCoolProp

# Compiler
#CC = g++ -Wall -O3
CC = emcc -O3

laine : laine.o text.o node.o polish.o matrix.o solver.o reduce.o
	$(CC) $^ -o $@ $(CPlib)

# Objects
text.o : text.cc text.hpp
	$(CC) -c $< -o $@ 

node.o : node.cc node.hpp
	$(CC) -c $< -o $@ $(CPlib) -fexceptions

polish.o : polish.cc polish.hpp 
	$(CC) -c $< -o $@ $(CPlib) -fexceptions

matrix.o : matrix.cc matrix.hpp 
	$(CC) -c $< -o $@ 

solver.o : solver.cc solver.hpp 
	$(CC) -c $< -o $@ $(CPlib) -fexceptions

reduce.o : reduce.cc reduce.hpp 
	$(CC) -c $< -o $@ $(CPlib) -fexceptions

laine.o : laine.cc 
	$(CC) -c $< -o $@ $(CPlib)

demo : wasm.cc text.o node.o polish.o matrix.o solver.o reduce.o
	$(CC) --bind ./src/wasm.cc text.o node.o polish.o matrix.o solver.o reduce.o -I./include -o ./laine.js $(CPlib) -s LLD_REPORT_UNDEFINED -s EXPORTED_FUNCTIONS='["_setThrew"]'  -s DISABLE_EXCEPTION_CATCHING=0 #-fexceptions -s INITIAL_MEMORY=33554432

# Utilities
.PHONY: clean
clean :
	rm laine *.o
