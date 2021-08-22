#include "reduce.hpp"

/**
 * Comparison for sorting equations
 */
bool lessVar::operator() (Node* first, Node* second){
  int nF = (first->findVars(*local)).size();
  int nS = (second->findVars(*local)).size();
  return (nF<nS);
}

/**
 * Verifies if a tree is "simple"
 */
bool simple(Node* tree,Scope &local){
  Node** childs = tree -> get_inputs();
  char lT = childs[0] -> get_type();
  if (lT == 'v'){
    std::string name = childs[0]->toString();
    if (local.find(name) == local.end()){
      StringSet vars = childs[1]->findVars(local);
      return (vars.find(name) == vars.end());
    } else{
      return false;
    }
  }
  return false;
}

/**
 * Removes simple equations from a forest
 */
std::vector<Node*> removeSimple(std::vector<Node*> &forest, Scope &local){
  std::vector<Node*> simpleEquations;
  StringSet names;
  for (unsigned i = 0; i<forest.size(); ++i){
    // Check if simple and erase
    if (simple(forest[i],local)){
      Node** inputs = forest[i] -> get_inputs();
      std::string name = inputs[0] -> toString();
      if (names.find(name) == names.end()){
	// Add name to subs
	simpleEquations.push_back(forest[i]);
	names.insert(name);
	// Erase from original
	forest.erase(forest.begin()+i);
	--i; // to avoid errors since tree is resized
      }
    }
  }
  return simpleEquations;
}

/**
 * Applies algebric substitutions in simple and other equations
 */
void algebraicSubs(std::vector<Node*> &simple, std::vector<Node*> &others, Scope &local){
  // Possible substitutions
  const unsigned n = simple.size();
  std::string *names = new std::string[n];
  Node** rightForest = new Node*[n];
  for (unsigned i=0; i<n; ++i){
    Node** inputs = simple[i] -> get_inputs();
    names[i] = inputs[0] -> toString();
    rightForest[i] = inputs[1];
  }

  // Table of substitutions
  bool* table = new bool[n*n];
  for (unsigned i = 0; i<n; ++i){
    for (unsigned j=0; j<n; ++j){
      table[i+j*n] = i==j ? false : true ;
    }
  }
  
  // Substitute in simple
  bool flag = true;
  while (flag){
    flag = false;
    for (unsigned i = 0; i<n; ++i){ // eq
      Node** inputs = simple[i] -> get_inputs();
      StringSet rightSide = inputs[1]->findVars(local);
      for (unsigned j = 0; j<simple.size(); ++j){ // eq - name/subs
	if (table[i+j*n] && (rightSide.find(names[j]) != rightSide.end())){
	  // Substitute var in 'j' by expression of 'j' in equation 'i'
	  inputs[1] -> swap_var(names[j],rightForest[j]);
	  rightForest[i] = inputs[1];
	  // Exclude this possibity
	  table[i+j*n] = false;
	  table[j+i*n] = false;
	  // Set flag
	  flag = true;
	} else{
	  continue;
	}
      }
    }
  }

  // Release memory
  delete[] table;

  // Substitute in the main equations;
  for (unsigned i = 0; i<others.size(); ++i){ // eq
    StringSet expression = others[i] -> findVars(local);
    for (unsigned j = 0; j<n; ++j){ // eq - name/subs
      if (expression.find(names[j]) != expression.end()){
	// Substitute var in 'j' by expression of 'j' in equation 'i'
	others[i] -> swap_var(names[j],rightForest[j]);
      } else{
	continue;
      }
    }
  }
}

/**
 * Separates equations into blocks and solve them
 */
void solveByBlocks(std::vector<Node*> &equations, Scope &solutions){
  lessVar condition(solutions); // Wrap Scope into lessVar
  
  while (!equations.empty()){    
    // Wrapped with scope (very important)
    std::sort(equations.begin(),equations.end(),condition);
    
    // Create a block
    StringSet varBlocks = equations[0] -> findVars(solutions);
    std::vector<Node*> block;

    // Verify if a lower block is possible
    if (varBlocks.size() < equations.size()){
      block.push_back(equations[0]);
      equations.erase(equations.begin());
    } else{
      std::swap(equations,block);
    }

    // Progressively add equations to the block
    while (varBlocks.size() != block.size()){
      for (unsigned i=0;i<equations.size(); ++i){
	// add a new set if it shares variables
	StringSet varEq = equations[i] -> findVars(solutions);
	for (auto &name:varEq){
	  if (varBlocks.find(name) != varBlocks.end()){
	    block.push_back(equations[i]);
	    varBlocks.insert(varEq.begin(),varEq.end());
	    equations.erase(equations.begin()+i);
	    --i;
	    break;
	  }
	}
	// check block size
	if (varBlocks.size() == block.size()){
	  break;
	}
      }
    }
    
    // Solve block
    int count = 0;
    const int max_count = 30;
    while (count < max_count){
      try{
	// Try first Brent and after Newton
	if (block.size() == 1 && count == 0){
	  solve(block[0],solutions);
	} else{
	  solve(block,solutions);
	}
	break;
      } catch (std::exception &e){
	++count;
	// Clear guesses
	for (const auto &name:varBlocks){
	  solutions.erase(name);
	}
	continue;
      }
    }
    if (count == max_count){
      throw std::invalid_argument("not converged @solveByBlocks");
    }
    
    // Release memory
    for(auto &eq:block){
      delete eq;
    }
  }
}

/**
 * Solves the problem
 */
void solveProblem(std::vector<std::string> &lines, Scope &solutions){
  /**
   * Get equations
   * Solve equations if possible, otherwise store it
   */
  std::vector<Node*> equations;
  for (unsigned j=0; j<lines.size(); ++j){
    Node* line = parse(lines[j]);
    StringSet lineVars = line -> findVars(solutions);
    // std::cout << "(" << j << ")" << "\t" << line -> toString() << std::endl;
    if (lineVars.size() == 1){
      solve(line,solutions);
      delete line; // clear memory
    } else{
      equations.push_back(line);
    }
  }

  /**
   * Split the problem
   * Simple equations can be excluded from the main problem
   */
  std::vector<Node*> simple;
  if (!equations.empty()){
    simple = removeSimple(equations,solutions);
    algebraicSubs(simple,equations,solutions);
  }

  /**
   * Blocks
   * Solve problem spliting into smaller blocks when possible
   */
  if(!equations.empty()){
    solveByBlocks(equations,solutions);
  }
  
  if(!simple.empty()){
    solveByBlocks(simple,solutions);
  }
}
