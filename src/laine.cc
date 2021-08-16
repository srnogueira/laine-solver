#include "text.hpp"   // input text and manipulation
#include "solver.hpp" // numerical solver
#include "reduce.hpp" // block solver and problem solver
#include <chrono>     // evaluation time


int main(){
  std::cout << "Laine | C++ console version" << std::endl;
  srand(time(NULL)); // seed for random numbers
  
  /**
   * Get file
   **/
  std::cout << "Filename: ";
  std::string filename;
  std::cin >> filename;
  std::vector<std::string> lines = getLines(filename);

  /**
   * Solve problem
   **/
  const auto t1 = std::chrono::high_resolution_clock::now(); // start chrono
  Scope solutions;
  solveProblem(lines,solutions);
  const auto t2 = std::chrono::high_resolution_clock::now();
  const auto ms_int= std::chrono::duration_cast<std::chrono::microseconds>(t2-t1);
  std::cout << "Time: "<< ms_int.count()/1e3<< " ms" << std::endl;

  /**
   * Print results
   **/
  for (auto &kv:solutions){
    std::cout << kv.first << ": " << kv.second << std::endl;
  }
  return true;
}

/* TO-DO
 * Create a equation class: similar to javascript
 * A "line" parser
 * Algebraic substitutions
 */
