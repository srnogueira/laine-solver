# Paths
VPATH = src

# Coolprop static library
CPIn = -I./lib/coolprop/include -I./lib/coolprop/externals/fmtlib/ -I./lib/coolprop/
CPlib = $(CPIn) -ldl -L./lib/coolprop/static -lCoolProp
EmccFlags = -s DISABLE_EXCEPTION_CATCHING=1 -s ERROR_ON_UNDEFINED_SYMBOLS=0 #-s EXPORTED_FUNCTIONS='["_setThrew"]'   

# Compiler
#CC = g++ -Wall -O3
CC = emcc -O2 --profiling

# Objects
text.o : text.cc text.hpp
	$(CC) -c $< -o $@ 

node.o : node.cc node.hpp
	$(CC) -c $< -o $@ $(CPIn) #-fexceptions

polish.o : polish.cc polish.hpp 
	$(CC) -c $< -o $@ $(CPIn) #-fexceptions

matrix.o : matrix.cc matrix.hpp 
	$(CC) -c $< -o $@ 

solver.o : solver.cc solver.hpp 
	$(CC) -c $< -o $@ $(CPIn) #-fexceptions

reduce.o : reduce.cc reduce.hpp 
	$(CC) -c $< -o $@ $(CPIn) #-fexceptions

laine.o : laine.cc 
	$(CC) -c $< -o $@ $(CPIn)

# Javascript (change compiler)
laine.js : wasm.cc text.o node.o polish.o matrix.o solver.o reduce.o
	$(CC) --bind $^ -o $@ $(CPlib) $(EmccFlags)

# C++ (change compiler)
laine : laine.o text.o node.o polish.o matrix.o solver.o reduce.o
	$(CC) $^ -o $@ $(CPlib)

# Utilities
.PHONY: clean
clean :
	rm *.o
