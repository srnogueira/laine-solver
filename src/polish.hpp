#ifndef _POLISH_
#define _POLISH_

#include <vector>    // vector
#include <stack>     // stack
#include <cctype>    // isdigit
#include <iostream>  // cout
#include "node.hpp"  // node

struct Token {
  char type;
  std::string letters;
  Token(const std::string name);
};
const int symbolType(const char symbol);
std::vector<Token> tokenize(const std::string line);

void addOperation(std::stack<char>& opStack, std::stack<Node*>& tkStack);
void addFunction(std::stack<std::string> &funStack,
		 std::stack<Node*> &inputStack,
		 std::stack<char> &opStack,
		 std::stack<Node*> &tkStack);

Node* parseTokens(const std::vector<Token> &tokens);
Node* parse(std::string line);

#endif 
