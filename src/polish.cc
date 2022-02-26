#include "polish.hpp" // prototypes

/**
 * NOT DO
 * Regex for tokenizer : too slow
 * Token for '=' : same stuff, more complicated
 **/

/**
 * Token constructor
 */
Token::Token(const std::string name){
  letters = name;
  const int code = symbolType(letters[0]);
  const char last = name.back();
  if (name.size() == 1){
    if (isdigit(name[0])){
      type = 'n'; // number
    } else{
      type = code == 0 ? 'v' : last; // var or operand
    }
  } else{
    if (isdigit(name[0]) || (code == 2 && isdigit(last))){
      type = 'n'; // number
    } else if (last == '\"' || last == '\''){
      type = 'w'; // word
    } else{
      type = 'v'; // variable or function
    }
  }
}

/**
 * Detects a delimiter
 */
const int symbolType(const char symbol) {
  int ans;
  switch (symbol){
  case '+': case '-':
    ans = 2;
    break;
  case '*': case '/':
    ans = 3;
    break;
  case '^':
    ans = 4;
    break;
  case '(': case ')': case ',':
    ans = 1;
    break;
  default:
    ans = 0;   // Number, variable, function or string
  }
  return ans;
}

/**
 * Separates the line into tokens
 */
std::vector<Token> tokenize(const std::string line){
  char c;
  int type;
  std::string letters = "";
  std::vector<Token> tokens;
  for (long unsigned int i=0; i<line.size(); ++i){
    c = line[i];
    type = symbolType(c);
    if (type == 0){
      if (c!=' ' && c!='\t'){
	// var or function, exclude whitespace
  	letters.push_back(c);
      }
    } else if (type == 2 &&
	       ((letters.empty() && (tokens.empty() || tokens.back().type=='(')) ||
		((letters.back()=='e' || letters.back()=='E') && isdigit(letters.front())))
	       ){
      if (isdigit(line[i+1])){
	// +number, -number, E+number, e-number
      	letters.push_back(c); 
      } else {
	// -var or +var in the begining
  	tokens.push_back(Token("0")); 
  	tokens.push_back(Token(std::string(1,c)));
      }
    } 
    else {
      // Push stored token (a number, word or variable)
      if (!letters.empty()){
  	tokens.push_back(Token(letters));
      }
      // Clear stored token
      letters.clear();
      // Push new token (operand)
      tokens.push_back(Token(std::string(1,c)));
    }
  }
  if (!letters.empty())
    tokens.push_back(Token(letters));
  
  // Debug
  // for (auto t:tokens)
  //   std::cout << t.letters << " [" << t.type <<"] "<< " ";
  // std::cout << std::endl;  
  return tokens;
}

/**
 * Include a operation to output
 */
void addOperation(std::stack<char>& opStack, std::stack<Node*>& tkStack){
  Node* right = tkStack.top();
  tkStack.pop();
  Node* left = tkStack.top();
  tkStack.pop();
  Node* tree;
  tree = new NodeOp(opStack.top(),left,right);
  opStack.pop();
  tkStack.push(tree);
}

/**
 * Include a function to output
 */
void addFunction(std::stack<std::string> &funStack,
		 std::stack<Node*> &inputStack,
		 std::stack<char> &opStack,
		 std::stack<Node*> &tkStack){
  const int n = inputStack.size();
  Node** inputs = new Node*[n];
  for (int i = n-1; i >= 0 ; --i){
    inputs[i] = inputStack.top();
    inputStack.pop();
  }
  Node* fun;
  fun = new NodeFun(funStack.top(),n,inputs);
  
  funStack.pop();
  tkStack.push(fun);
  opStack.pop();
}

/**
 * Parses tokens into tree
 */
Node* parseTokens(const std::vector<Token> &tokens){
  char code;
  int level;
  std::string letters;
  std::stack<Node*> tkStack;
  std::stack<Node*> inputStack;
  std::stack<char> opStack;
  std::stack<std::string> funStack;
  for(long unsigned int i=0 ; i<tokens.size() ; ++i){
    code = tokens[i].type;
    letters = tokens[i].letters;
    switch (code){
    case 'n':
      {
	// Number
	Node* numbNode = new NodeDouble(stod(letters));
	tkStack.push(numbNode);
      }
      break;
    case 'w':
      {
	// Word - obs: you have to remove ' or "
	letters = letters.substr(1,letters.length()-2);
	Node* wordNode = new NodeString(letters);
	tkStack.push(wordNode);
      }
      break;
    case 'v':
      {
	if (i+1 != tokens.size() && tokens[i+1].letters == "("){
	  // Function
	  funStack.push(letters);
	  opStack.push('f');
	} else{
	  // Variable (exclude functions for now)
	  Node* var = new NodeVar(letters);
	  tkStack.push(var);
	}
      }
      break;
    case '(':
      {
	opStack.push(code);
      }
      break;
    case ',':
      {
	// Store function inputs
	while (opStack.top() != '('){
	  addOperation(opStack,tkStack);
	}
	inputStack.push(tkStack.top());
	tkStack.pop();
      }
      break;
    case ')':
      {
	while (opStack.top() != '('){
	  addOperation(opStack,tkStack);
	}
	opStack.pop(); // pop left parenthesis
	if (!funStack.empty() && opStack.top()=='f'){
	  // Add function
	  inputStack.push(tkStack.top());
	  tkStack.pop();
	  addFunction(funStack,inputStack,opStack,tkStack);
	}
      }
      break;
    default:
      {
	// Operation
	level = symbolType(code);
	while (!opStack.empty() && opStack.top() != '(' &&
	       (symbolType(opStack.top()) > level || ( (symbolType(opStack.top()) == level) && code != '^'))){
	  addOperation(opStack,tkStack);
	}
	opStack.push(code);
      }
      break;
    }
  } 
  // Operators tokens lefts
  while(!opStack.empty()){
    addOperation(opStack,tkStack);
  }
  // Debug
  //std::cout << tkStack.top() -> toString() << std::endl;
  return tkStack.top();
}

/**
 * Parses a string into a tree
 */
Node* parse(std::string line){
  Node* tree = parseTokens(tokenize(line));
  return tree;
}

/**
 * Solution with regular expressions
 * Much slower than this custom function
 **/
// #include <regex>
// std::regex e("([0-9]*\\.?[0-9]+)|(\\+|\\-|\\(|\\)|\\*|\\/|\\^)|\\w+");  
// std::smatch sm;
// std::string searched = line;
// while (std::regex_search (searched, sm, e)){
//   tokens.push_back(Token(sm[0]));
//   searched = sm.suffix();
// }
