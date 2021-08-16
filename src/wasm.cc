#include "text.hpp"   // input text and manipulation
#include "solver.hpp" // numerical solver
#include "reduce.hpp" // block solver and problem solver
#include <emscripten/bind.h> // wasm

/* *
 * Evaluates the problem from a string
 * Function call for wasm
 */
std::string solveText(std::string text){
  /**
   * Break into lines
   **/
  std::vector<std::string> lines;
  while (!text.empty()){
    std::size_t pos = text.find('\n');
    if (pos != std::string::npos){
      lines.push_back(text.substr(0,pos));
      if (pos != text.size()){
	text = text.substr(pos+1);
      } else{
	text.clear();
      }
    } else{
      lines.push_back(text);
      text.clear();
    }    
  }
  
  /**
   * Get expressions
   **/
  std::vector<std::string> linesClear;
  std::string subline;
  for (auto &line:lines){
    while (!line.empty()){
      subline = breakLines(line);
      if (!subline.empty()){
	linesClear.push_back(minusExp(subline));
      }
    }
  }

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

int main(){
  return 0;
}
