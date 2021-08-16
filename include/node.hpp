#include <string>    // strings
#include <cmath>     // math functions
#include <map>       // store variables
#include <set>       // sets variables names
#include <stdexcept> // exceptions
// #include "CoolProp.h"

/**
 * TO-DO
 * Automatic prune nodes that can be computed
 *
 * NOT IMPORTANT
 * Generalize NodeOp e NodeFun
 * Divide functions per number of inputs : non-type template parameters
 * Enum function names and include more functions
 * makes evalFun accept doubles and put everything in another header file
 *
 * NOT-DO
 * Variable = "string" (implement as a equation parser to scope)
 * User-defined functions (implement as a equation parser to scope)
 */

#ifndef _NODE_
#define _NODE_

/**
 * Scope
 **/
typedef std::map<std::string,double> Scope;
Scope blankScope; // default map

/**
 * Default functions
 **/
std::map<std::string,int> funCodes =
  {
   {"exp", '1'},
   {"log", '2'},
   {"log10", '3'},
   {"abs",'4'},
   {"pow",'5'},
   {"PropsSI",'6'}
  };

std::map<int,std::string> funNames =
  {
   {'1', "exp"},
   {'2', "log"},
   {'3', "log10"},
   {'4',"abs"},
   {'5',"pow"},
   {'6',"PropsSI"}
  };

using StringSet = std::set<std::string>;

/**
 * Abstract Node 
 **/
class Node {
public:
  virtual ~Node()=default;
  virtual char get_type() {return ' ';}
  virtual char get_op() {return ' ';}
  virtual int get_n() {return 0;}
  virtual Node** get_inputs() {return nullptr;}
  virtual double eval(Scope &local){return 0;}
  virtual StringSet findVars(Scope &local=blankScope){return StringSet();}
  virtual std::string toString(){return "";}
  virtual Node* get_copy(){return nullptr;}
  virtual void swap_var(std::string var, Node* tree){};
};

/**
 * Evaluates a operation
 **/
double evalOp(const double left,const double right,const char op){
  double ans;
  switch (op){
  case '+':
    ans = left+right;
    break;
  case '-':
    ans = left-right;
    break;
  case '*':
    ans = left*right;
    break;
  case '/':
    ans = left/right;
    break;
  case '^':
    ans = pow(left,right);
    break;
  default:
    throw std::invalid_argument("op @evalOP");
  }
  return ans;
}

/**
 * Evaluates a function
 **/
double evalFun(const char code,Node** input, Scope &local){
  double ans;
  switch (code){
  case '1':
    ans = exp(input[0] -> eval(local));
    break;
  case '2':
    ans = log(input[0] -> eval(local));
    break;
  case '3':
    ans = log10(input[0] -> eval(local));
    break;
  case '4':
    ans = abs(input[0] -> eval(local));
    break;
  case '5':
    ans = pow(input[0] -> eval(local), input[1] -> eval(local));
    break;
  case '6':
    {
      // std::string p = input[0]->toString();
      // std::string v1 = input[1]->toString();
      // double n1 = input[2] -> eval(local);
      // std::string v2 = input[3]->toString();
      // double n2 = input[4] -> eval(local);
      // std::string fluid = input[5]->toString();
      ans = 0;//CoolProp::PropsSI(p,v1,n1,v2,n2,fluid);
    }
    break;
  default:
    throw std::invalid_argument("code @evalFun");
  }
  return ans;
}

/**
 * NodeDouble - stores a double
 **/
class NodeDouble : public Node {
  double value;
public:
  NodeDouble(double input){value = input;}
  virtual char get_type() {return 'n';}
  virtual double eval(Scope &local=blankScope) {return value;}
  virtual std::string toString(){return std::to_string(value);}
  virtual NodeDouble* get_copy(){return new NodeDouble(eval());}
};

/**
 * NodeString - stores a string
 **/
class NodeString: public Node {
  std::string word;
public:
  NodeString(std::string input){word = input;}
  virtual char get_type(){return 'w';}
  virtual std::string toString(){return word;}
  virtual NodeString* get_copy(){return new NodeString(word);}
};

/**
 * Variable Node
 * variables are double values stored in Scope
 **/
class NodeVar : public Node {
  std::string name;
public:
  NodeVar(std::string input){name = input;}
  virtual char get_type(){return 'v';}  
  virtual double eval(Scope &local){return local[name];}
  virtual std::string toString(){return name;}
  virtual NodeVar* get_copy(){return new NodeVar(name);}
  virtual StringSet findVars(Scope &local=blankScope){
    StringSet names;
    if (local.find(name) == local.end()){
      names.insert(name);
    }
    return names;
  }
};

/* *
 * Operation Node: + - * / ^
 */
class NodeOp : public Node {
  char op;
  Node** inputs;
public:
  NodeOp(char symbol, Node* a, Node* b){
    op = symbol;
    inputs = new Node*[2];
    inputs[0] = a;
    inputs[1] = b;
  }
  virtual ~NodeOp(){
    delete inputs[0]; // required!
    delete inputs[1]; // required!
    delete[] inputs;
  }
  virtual char get_type(){return 'o';}
  virtual char get_op(){return op;}
  Node** get_inputs(){return inputs;}
  virtual double eval(Scope &local){
    const double left = inputs[0] -> eval(local);
    const double right = inputs[1] -> eval(local);
    return evalOp(left,right,op);
  }
  virtual StringSet findVars(Scope &local=blankScope){
    StringSet names;
    std::set<std::string> dummy;
    for (int i=0;i<2;++i){
      dummy = inputs[i] -> findVars(local);
      names.insert(dummy.begin(),dummy.end());
    }
    return names;
  }
  virtual std::string toString(){
    std::string out;
    out+= inputs[0] -> toString() + ' ' ;
    out+= inputs[1] -> toString() + ' ' ;
    out+= get_op();
    out+= ' ';
    return out;
  }
  virtual NodeOp* get_copy(){
    Node* left = inputs[0]->get_copy();
    Node* right = inputs[1]->get_copy();
    return new NodeOp(op,left,right);
  }
  virtual void swap_var(std::string var, Node* tree){
    for (int i = 0; i<2 ; ++i){
      char type = inputs[i] ->get_type();
      if (type == 'v' && inputs[i]->toString() == var){
	// Create a copy, swap and delete old;
	Node* copy = tree ->get_copy();
	std::swap(inputs[i],copy);
	delete copy;
      } else if (type == 'o' || type == 'f'){
	inputs[i] -> swap_var(var,tree);
      }
    }
  }
};

/**
 * Function Node
 **/
class NodeFun : public Node{
  char op;
  int n;
  Node** inputs;
public:
  NodeFun(std::string alias, int number, Node** var){
    op = funCodes[alias];
    n = number;
    inputs = new Node*[n];
    for (int i=0; i<n; ++i){
      inputs[i] = var[i];
    }
  }
  virtual ~NodeFun(){
    int n = get_n();
    for (int i=0;i<n;++i){
      delete inputs[i];
    }
    delete[] inputs;
  }
  virtual char get_type(){return 'f';}
  virtual char get_op(){return op;}
  virtual int get_n(){return n;}
  virtual Node** get_inputs(){return inputs;}
  virtual double eval(Scope &local){return evalFun(op,inputs,local);}
  virtual StringSet findVars(Scope &local=blankScope){
    StringSet names;
    std::set<std::string> dummy;
    for (int i=0;i<n;++i){
      dummy = inputs[i] -> findVars(local);
      names.insert(dummy.begin(),dummy.end());
    }
    return names;
  }
  virtual std::string toString(){
    std::string out;
    for (int i = 0; i<n; ++i){
      out += inputs[i] -> toString()+ ' ';
    }
    out += funNames[op] + ' '; 
    return out;
  }
  virtual NodeFun* get_copy(){
    Node** copy_inputs = new Node*[n];
    for (int i=0;i<n;++i){
      copy_inputs[i] = inputs[i]->get_copy();
    }
    return new NodeFun(funNames[op], n, copy_inputs);
  }
  virtual void swap_var(std::string var, Node* tree){
    for (int i = 0; i<n ; ++i){
      char type = inputs[i] ->get_type();
      if (type == 'v' && inputs[i]->toString() == var){
	// Create a copy, swap and delete old;
	Node* copy = tree ->get_copy();
	std::swap(inputs[i],copy);
	delete copy;
      } else if (type == 'o' || type == 'f'){
	inputs[i] -> swap_var(var,tree);
      }
    }
  }
};

#endif // _NODE_
