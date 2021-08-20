# Paths
vpath %.cc ./src
vpath %.hpp ./include

# Coolprop
CPinclude = -ldl -L./lib/coolprop/static -I./lib/coolprop/include -I./lib/coolprop/externals/fmtlib/

# .hpp files
H_SOURCE = $(wildcard ./include/*.hpp)

laine : laine.cc $(H_SOURCE)
	g++ -Wall -O3 $(CPinclude) ./src/laine.cc -I./include -o ./bin/laine -lCoolProp

l : laine.cc $(H_SOURCE)
	g++ -Wall $(CPinclude) ./src/laine.cc -I./include -o ./bin/laine -lCoolProp

demo : wasm.cc $(H_SOURCE)
	emcc --bind -O3 ./src/wasm.cc -I./include -o ./laine.js

# Old configurations for CoolProp integration
# CPinclude = -ldl -L./lib/coolprop/static -I./lib/coolprop/include -I./lib/coolprop/externals/fmtlib/
# laine : laine.cc text.hpp solver.hpp polish.hpp matrix.hpp node.hpp reduce.hpp
# 	g++ -Wall -O3 $(CPinclude) laine.cc -o laine -lCoolProp
