#include "text.hpp"   // input text and manipulation
#include "solver.hpp" // numerical solver
#include "reduce.hpp" // block solver and problem solver
#include <emscripten/bind.h> // wasm
#include <emscripten.h> // wasm

/* *
 * Evaluates the problem from a string
 * Function call for wasm
 */
std::string solveText(std::string text){
  srand(time(NULL)); // seed for random numbers

  // Get expressions lines
  std::vector<std::string> lines = getLinesFromText(text);
  // Extract only problem lines and keep guess lines without "?"
  // Parse all guess lines as a problem and store the solution
  // Use these stored solutions to alter the behaviour of findGuesses

  
  /**
   * Solve
   **/
  Scope solutions;
  solveProblem(linesClear,solutions);
  
  // give solution
  std::string res="{";
  unsigned i = 0;
  for (auto kv:solutions){
    res += "\""+ kv.first + "\" : " + std::to_string(kv.second);
    if (i < solutions.size()-1){
      res += ",";
    }
    else{
      res += "}";
    }
    i++;
  }
  return res;
}

// compile with: emcc --bind -o wasm.html wasm.cc
using namespace emscripten;

EMSCRIPTEN_BINDINGS(my_module) {
  function("laine", &solveText);
}

std::string getExceptionMessage(intptr_t exceptionPtr) {
  return std::string(reinterpret_cast<std::exception *>(exceptionPtr)->what());
}

EMSCRIPTEN_BINDINGS(Bindings) {
  emscripten::function("getExceptionMessage", &getExceptionMessage);
};

