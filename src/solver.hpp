#ifndef _SOLVER_
#define _SOLVER_

#include <iostream>  // in-out
#include <algorithm> // remove
#include <time.h>    // random
#include <math.h>    // isfinite
#include <chrono>     // evaluation time

#include "polish.hpp" // expression parser
#include "matrix.hpp" // matrix -> correct the index

/**
 * Variables
 * stores names and a table for faster jacobians
 */
struct Variables{
  StringSet all;
  bool* table; // if a equation has or not a variable name
  int n;
  Variables(const std::vector<Node*> &forest, Scope &local);
  ~Variables(){delete[] table;};
  Variables(const Variables &original);
};

double dfdx(Node* &tree, Scope &guess, std::string name, double y);
void evalForest(const std::vector<Node*> &forest, Scope &guess, mat &answers, mat &side);

void evalJacobian(std::vector<Node*> &forest, Scope &guess,const Variables &vars, mat &jac, mat &answers);
void evalBroyden(mat &jac, mat &dx, mat &df);

void updateScope(Scope &guess,const Variables &vars, const mat &guessN);
  
double evalError(const mat &answers);
double evalError(const mat &answers,const mat &side);
double evalError(const mat &guessN, Variables &vars, std::vector<Node*> &forest, Scope &guessScope);

using Guess = std::pair<mat,double>;
bool lessError(Guess first, Guess second);

std::vector<Guess> findGuess(Variables &vars, std::vector<Node*> &forest, Scope &guessScope);
std::vector<Guess> findGuessPair(Variables &vars, std::vector<Node*> &forest, Scope &guessScope);

mat brent(std::string var, Node* tree, Scope &guessScope);
bool solve(Node* tree, Scope &guessScope);
bool solve(std::vector<Node*> &forest, Scope &guessScope, unsigned i);

#endif //_SOLVER_
