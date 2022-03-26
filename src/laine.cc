#include "text.hpp"   // input text and manipulation
#include "reduce.hpp" // block solver and problem solver
#include <chrono>     // evaluation time

int main(){
  std::cout << "Laine | C++ console version" << std::endl;
  srand(time(NULL)); // seed for random numbers
  
  while (true){
      
    /**
     * Get file
     **/
    std::cout << "Filename: ";
    std::string filename;
    std::cin >> filename;
    std::vector<std::string> lines = getLines(filename);
    
    while (true){
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

      // Repeat?
      char solveAgain = 'x';
      while (solveAgain != 'y' && solveAgain != 'n'){
	std::cout << "Solve again? (y/n)" << std::endl;
	std::cin >> solveAgain;
      }
      if (solveAgain == 'n'){
	break; // solve loop
      }
      
    }

    // New file?
    char newFile = 'x';
    while (newFile != 'y' && newFile != 'n'){
      std::cout << "Solve another problem? (y/n)" << std::endl;
      std::cin >> newFile;
    }    
    if (newFile == 'n'){
      break; // new file loop
    }
    
  }
  return true;
}

/* TO-DO
 * A "line" parser
 */
