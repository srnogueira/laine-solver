#include "node.hpp" // prototypes

//#include <emscripten.h> // wasm

/**
 * TO-DO
 * Automatic prune nodes that can be computed
 * Add units, arrays and others
 * String variables could use the function toString to return the value, multiple scopes
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
double evalFunOne(const unsigned char code,Node** input,Scope &local){
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
 * Functions with multiples inputs
 */

std::map<std::string,unsigned char> funsMore =
  {
   {"HAPropsSI",0},{"Props1SI",1}
  };

std::map<unsigned char,std::string> namesMore =
  {
   {0,"HAPropsSI"},{1,"Props1SI"}
  };

double evalFunMore(const unsigned char code, Node** input, Scope &local){
  double ans;
  switch (code){
  case 0:
    {
      std::string p = input[0]->toString();
      std::string v1 = input[1]->toString();
      double n1 = input[2] -> eval(local);
      std::string v2 = input[3]->toString();
      double n2 = input[4] -> eval(local);
      std::string v3 = input[5]->toString();
      double n3 = input[6] -> eval(local);
      ans = HumidAir::HAPropsSI(p,v1,n1,v2,n2,v3,n3);
      // OBS: CoolProp library is compiled without error report
    }
    break;
  case 1:
    {
      std::string p = input[0]->toString();
      std::string fluid = input[1]->toString();
      ans = CoolProp::Props1SI(p,fluid);
      // OBS: CoolProp library is compiled without error report
    }
    break;
  default:
    throw std::invalid_argument("code @evalFunOne");
  }
  return ans;
}

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
    //return 0;
    return evalFunMore(op,inputs,local);
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
  double n1 = inputs[0] -> eval(local);
  double n2 = inputs[1] -> eval(local);
  switch (op) {
  case '+':
    return n1+n2;
  case '-':
    return n1-n2;
  case '*':
    return n1*n2;
  case '/':
    return n1/n2;
  case '^':
    return pow(n1,n2);
  default:
    throw std::invalid_argument("op @evalOp");
  }
  return 0;
}

/**
 * NodeFun give string
 */
std::string NodeOp::toString(){
  std::string out;
  for (int i = 0; i<n; ++i){
    out += inputs[i] -> toString()+ ' ';
  }
  out += op;
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

/**
 * Node CoolProp
 */
NodePropsSI::NodePropsSI(std::string alias, int number, Node** var){
  // Copied from NodeFun
  n = number;
  op = n == 1 ? funsOne[alias] : funsMore[alias];
  inputs = new Node*[n];
  for (int i=0; i<n; ++i){
    inputs[i] = var[i];
  }

  // To reduce computational time required for checks
  std::string v1 = inputs[1]->toString();
  std::string v2 = inputs[3]->toString();
  std::string fluid = inputs[5]->toString();
  if (v1 == "Q" || v2 == "Q"){
    TMAX = CoolProp::PropsSI("TCRIT","",0,"",0,fluid);
    PMAX = CoolProp::PropsSI("PCRIT","",0,"",0,fluid);
  } else {
    TMAX = CoolProp::PropsSI("TMAX","",0,"",0,fluid);
    PMAX = CoolProp::PropsSI("PMAX","",0,"",0,fluid);
  }
  TMIN = CoolProp::PropsSI("TMIN","",0,"",0,fluid);
  PMIN = CoolProp::PropsSI("PMIN","",0,"",0,fluid);
}

NodePropsSI::NodePropsSI(Node** in, double Tmax, double Pmax, double Tmin, double Pmin){
  // Copied from NodeFun
  n = 6;
  op = 0;
  inputs = new Node*[n];
  for (int i=0; i<n; ++i){
    inputs[i] = in[i];
  }

  // To avoid recalculations
  TMAX = Tmax;
  PMAX = Pmax;
  TMIN = Tmin;
  PMIN = Pmin;
}

double NodePropsSI::eval(Scope &local){
  // Get data
  std::string p = inputs[0]->toString();
  std::string v1 = inputs[1]->toString();
  double n1 = inputs[2] -> eval(local);
  std::string v2 = inputs[3]->toString();
  double n2 = inputs[4] -> eval(local);
  std::string fluid = inputs[5]->toString();

  // Valid values
  if (!std::isfinite(n1) || !std::isfinite(n2)){
    return NAN;
  }
  
  //CoolProp::set_config_bool(DONT_CHECK_PROPERTY_LIMITS,true);
  // Check temperature and pressure limits
  // Temperature and pressure limits
  if (v1 == "T"){
    if (n1 >= TMAX || n1 <= TMIN){
      return NAN;
    } else if (v2 == "P"){
      if (n2 >= PMAX || n2 <= PMIN){
	return NAN;
      }
    }
  } else if (v2 == "T"){
    if (n2 >= TMAX || n2 <= TMIN){
      return NAN;
    } else if (v1 == "P"){
      if (n1 >= PMAX || n1 <= PMIN){
	return NAN;
      }
    }
  }

  double ans = CoolProp::PropsSI(p,v1,n1,v2,n2,fluid);

  if (std::isinf(ans)){
    return NAN;
  } else {
    return ans;
  }
}

/**
 * NodeFun copy
 */
NodePropsSI* NodePropsSI::get_copy(){
  Node** copy_inputs = new Node*[n];
  for (int i=0;i<n;++i){
    copy_inputs[i] = inputs[i]->get_copy();
  }
  std::string alias = n == 1 ? namesOne[op] : namesMore[op];
  return new NodePropsSI(copy_inputs, TMAX, PMAX, TMIN, PMIN);
}
