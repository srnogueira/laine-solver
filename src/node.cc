#include "CoolProp.h"     // PropsSI
#include "HumidAirProp.h" // HAPropsSI

#include "node.hpp" // prototypes

/**
 * TO-DO
 * Automatic prune nodes that can be computed
 *
 * NOT IMPORTANT
 * Divide functions per number of inputs : non-type template parameters
 * Enum function names and include more functions
 * makes evalFun accept doubles and put everything in another header file
 *
 * NOT-DO
 * Variable = "string" (implement as a equation parser to scope)
 * User-defined functions (implement as a equation parser to scope)
 */

/**
 * NodeVar find variables
 */
StringSet NodeVar::findVars(Scope &local){
  StringSet names;
  if (local.find(name) == local.end()){
    names.insert(name);
  }
  return names;
}

/**
 * Default functions
 */
std::map<std::string,int> funCodes =
  {
   {"exp", '1'},
   {"log", '2'},
   {"log10", '3'},
   {"abs",'4'},
   {"pow",'5'},
   {"PropsSI",'6'},
   {"HAPropsSI",'7'}
  };

std::map<int,std::string> funNames =
  {
   {'1', "exp"},
   {'2', "log"},
   {'3', "log10"},
   {'4',"abs"},
   {'5',"pow"},
   {'6',"PropsSI"},
   {'7',"HAPropsSI"}
  };

/**
 * Evaluates a function
 */
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
      std::string p = input[0]->toString();
      std::string v1 = input[1]->toString();
      double n1 = input[2] -> eval(local);
      std::string v2 = input[3]->toString();
      double n2 = input[4] -> eval(local);
      std::string fluid = input[5]->toString();
      ans = CoolProp::PropsSI(p,v1,n1,v2,n2,fluid);
    }
    break;
  case '7':
    {
      std::string p = input[0]->toString();
      std::string v1 = input[1]->toString();
      double n1 = input[2] -> eval(local);
      std::string v2 = input[3]->toString();
      double n2 = input[4] -> eval(local);
      std::string v3 = input[5]->toString();
      double n3 = input[6] -> eval(local);
      ans = HumidAir::HAPropsSI(p,v1,n1,v2,n2,v3,n3);
    }
    break;
  default:
    throw std::invalid_argument("code @evalFun");
  }
  return ans;
}

/**
 * NodeFun constructor
 */
NodeFun::NodeFun(std::string alias, int number, Node** var){  
  op = funCodes[alias];
  n = number;
  inputs = new Node*[n];
  for (int i=0; i<n; ++i){
    inputs[i] = var[i];
  }
}

/**
 * NodeFun destructor
 */
NodeFun::~NodeFun(){
  int n = get_n();
  for (int i=0;i<n;++i){
    delete inputs[i];
  }
  delete[] inputs;
}

/**
 * NodeFun find vars
 */
StringSet NodeFun::findVars(Scope &local){
  StringSet names;
  std::set<std::string> dummy;
  for (int i=0;i<n;++i){
    dummy = inputs[i] -> findVars(local);
    names.insert(dummy.begin(),dummy.end());
  }
  return names;
}

/**
 * NodeFun give string
 */
std::string NodeFun::toString(){
  std::string out;
  for (int i = 0; i<n; ++i){
    out += inputs[i] -> toString()+ ' ';
  }
  out += funNames[op] + ' '; 
  return out;
}

/**
 * NodeFun copy
 */
NodeFun* NodeFun::get_copy(){
  Node** copy_inputs = new Node*[n];
  for (int i=0;i<n;++i){
    copy_inputs[i] = inputs[i]->get_copy();
  }
  return new NodeFun(funNames[op], n, copy_inputs);
}

/**
 * NodeFun swap variables
 */
void NodeFun::swap_var(std::string var, Node* tree){
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

/**
 * NodeOp constructor
 */
NodeOp::NodeOp(char symbol, Node* a, Node* b){
  op = symbol;
  n = 2;
  inputs = new Node*[2];
  inputs[0] = a;
  inputs[1] = b;
}

/**
 * NodeOp eval
 */
double NodeOp::eval(Scope &local){
  const double left = inputs[0] -> eval(local);
  const double right = inputs[1] -> eval(local);
  return evalOp(left,right,op);
}

/**
 * Evaluates a operation
 */
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
 * NodeOp to string
 */
std::string NodeOp::toString(){
  std::string out;
  out+= inputs[0] -> toString() + ' ' ;
  out+= inputs[1] -> toString() + ' ' ;
  out+= get_op();
  out+= ' ';
  return out;
}

/**
 * NodeOp copy
 */
NodeOp* NodeOp::get_copy(){
  Node* left = inputs[0]->get_copy();
  Node* right = inputs[1]->get_copy();
  return new NodeOp(op,left,right);
}
