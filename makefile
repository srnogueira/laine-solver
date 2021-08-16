vpath %.cc ./src
vpath %.hpp ./include

# .hpp files
H_SOURCE = $(wildcard ./include/*.hpp)

laine : laine.cc $(H_SOURCE)
	g++ -Wall -O3 ./src/laine.cc -I./include -o ./bin/laine

wasm : wasm.cc $(H_SOURCE)
	emcc --bind -O3 ./src/wasm.cc -I./include -o ./demo/laine.js

# Old configurations for CoolProp integration
# CPinclude = -ldl -L./lib/coolprop/static -I./lib/coolprop/include -I./lib/coolprop/externals/fmtlib/
# laine : laine.cc text.hpp solver.hpp polish.hpp matrix.hpp node.hpp reduce.hpp
# 	g++ -Wall -O3 $(CPinclude) laine.cc -o laine -lCoolProp
