// #include "CoolProp.h"     // PropsSI
// #include "HumidAirProp.h" // HAPropsSI

#include "node.hpp" // prototypes

/**
 * TO-DO
 * Automatic prune nodes that can be computed
 *
 * NOT-DO
 * Variable = "string" (implement as a equation parser to scope)
 * User-defined functions (implement as a equation parser to scope)
 * makes evalFun accept doubles and put everything in another header file
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
std::map<std::string,unsigned char> funsOne =
  {
   {"exp",0},{"log",1},{"log10",2},{"fabs",3},{"cos",4},
   {"sin",5},{"tan",6},{"sqrt",7},{"acos",8},{"asin",9},
   {"atan",10},{"cosh",11},{"sinh",12},{"tanh",13}
  };

std::map<unsigned char,std::string> namesOne =
  {
   {0,"exp"},{1,"log"},{2,"log10"},{3,"fabs"},{4,"cos"},
   {5,"sin"},{6,"tan"},{7,"sqrt"},{8,"acos"},{9,"asin"},
   {10,"atan"},{11,"cosh"},{12,"sinh"},{13,"tanh"}
  };

/**
 * Evaluates a function
 */
double evalFunOne(const unsigned char code,Node** input,Scope local){
  double value = input[0] -> eval(local);
  double ans;
  switch (code){
  case 0:
    ans = exp(value);
    break;
  case 1:
    ans = log(value);
    break;
  case 2:
    ans = log10(value);
    break;
  case 3:
    ans = fabs(value);
    break;
  case 4:
    ans = cos(value);
    break;
  case 5:
    ans = sin(value);
    break;
  case 6:
    ans = tan(value);
    break;
  case 7:
    ans = sqrt(value);
    break;
  case 8:
    ans = acos(value);
    break;
  case 9:
    ans = asin(value);
    break;
  case 10:
    ans = atan(value);
    break;
  case 11:
    ans = cosh(value);
    break;
  case 12:
    ans = sinh(value);
    break;
  case 13:
    ans = tanh(value);
    break;
  default:
    throw std::invalid_argument("code @evalFunOne");
  }
  return ans;
}

/**
 * CoopProp functions
 */
std::map<std::string,unsigned char> funsMore =
  {
   {"PropsSI",0},{"HAPropsSI",1},{"Props1SI",2}
  };

std::map<unsigned char,std::string> namesMore =
  {
   {0,"PropsSI"},{1,"HAPropsSI"},{2,"Props1SI"}
  };

// double evalFunMore(const unsigned char code, Node* input, Scope local){
//   double ans;
//   switch (code){
//   case 0:
//     {
//       std::string p = input[0]->toString();
//       std::string v1 = input[1]->toString();
//       double n1 = input[2] -> eval(local);
//       std::string v2 = input[3]->toString();
//       double n2 = input[4] -> eval(local);
//       std::string fluid = input[5]->toString();
//       ans = CoolProp::PropsSI(p,v1,n1,v2,n2,fluid);
//     }
//     break;
//   case 1:
//     {
//       std::string p = input[0]->toString();
//       std::string v1 = input[1]->toString();
//       double n1 = input[2] -> eval(local);
//       std::string v2 = input[3]->toString();
//       double n2 = input[4] -> eval(local);
//       std::string v3 = input[5]->toString();
//       double n3 = input[6] -> eval(local);
//       ans = HumidAir::HAPropsSI(p,v1,n1,v2,n2,v3,n3);
//     }
//     break;
//   case 2:
//     {
//       std::string p = input[0]->toString();
//       std::string fluid = input[1]->toString();
//       ans = CoolProp::Props1SI(p,fluid);
//     }
//     break;
//   default:
//     throw std::invalid_argument("code @evalFunOne");
//   }
// }

/**
 * NodeFun constructor
 */
NodeFun::NodeFun(std::string alias, int number, Node** var){  
  n = number;
  op = n == 1 ? funsOne[alias] : funsMore[alias];
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
 * NodeFun eval
 */
double NodeFun::eval(Scope &local){
  if (n==1){
    return evalFunOne(op,inputs,local);
  } else{
    return 0;
    //return evalFunMore(op,inputs,local);
  }
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
  if (n==1){
    out += namesOne[op] + ' ';
  } else{
    out += namesMore[op] + ' ';
  }
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
  std::string alias = n == 1 ? namesOne[op] : namesMore[op];
  return new NodeFun(alias, n, copy_inputs);
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
