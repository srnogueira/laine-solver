#ifndef _REDUCE_
#define _REDUCE_

#include "solver.hpp"

struct lessVar {
  Scope* local;
  lessVar(Scope& input){local = &input;}
  bool operator() (Node* first, Node* second);
};

bool simple(Node* tree,Scope &local);
std::vector<Node*> removeSimple(std::vector<Node*> &forest, Scope &local);
void algebraicSubs(std::vector<Node*> &simple, std::vector<Node*> &others, Scope &local);

void solveByBlocks(std::vector<Node*> &equations, Scope &solutions);
void solveProblem(std::vector<std::string> &lines, Scope &solutions);

#endif
