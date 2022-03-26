#ifndef _NODE_
#define _NODE_

#include <string>         // strings
#include <cmath>          // math functions
#include <map>            // store variables
#include <set>            // sets variables names
#include <stdexcept>      // exceptions

#include "CoolProp.h"     // PropsSI
#include "AbstractState.h"     // PropsSI
#include "HumidAirProp.h" // HAPropsSI

// Scope
typedef std::map<std::string,double> Scope;

// StringSet
using StringSet = std::set<std::string>;

/**
 * Abstract Node 
 */
class Node {
public:
  virtual ~Node()=default;
  virtual char get_type() {return ' ';}
  virtual char get_op() {return ' ';}
  virtual int get_n() {return 0;}
  virtual Node** get_inputs() {return nullptr;}
  virtual double eval(Scope &local){return 0;}
  virtual StringSet findVars(Scope &local){return StringSet();}
  virtual std::string toString(){return "";}
  virtual Node* get_copy(){return nullptr;}
  virtual void swap_var(std::string var, Node* tree){};
};

double evalOp(const double left,const double right,const char op);

/**
 * NodeDouble - stores a double
 */
class NodeDouble : public Node {
  double value;
public:
  NodeDouble(double input){value = input;}
  virtual char get_type() {return 'n';}
  virtual double eval(Scope &local) {return value;}
  virtual std::string toString(){return std::to_string(value);}
  virtual NodeDouble* get_copy(){return new NodeDouble(value);}
};

/**
 * NodeString - stores a string
 */
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
 */
class NodeVar : public Node {
  std::string name;
public:
  NodeVar(std::string input){name = input;}
  virtual char get_type(){return 'v';}  
  virtual double eval(Scope &local){return local[name];}
  virtual std::string toString(){return name;}
  virtual NodeVar* get_copy(){return new NodeVar(name);}
  virtual StringSet findVars(Scope &local);
};

/**
 * Function Node
 */
class NodeFun : public Node{
protected:
  char op;
  int n;
  Node** inputs;
public:
  NodeFun(std::string alias, int number, Node** var);
  NodeFun()=default;
  virtual ~NodeFun();
  virtual char get_type(){return 'f';}
  virtual char get_op(){return op;}
  virtual int get_n(){return n;}
  virtual Node** get_inputs(){return inputs;}
  virtual double eval(Scope &local);
  virtual StringSet findVars(Scope &local);
  virtual std::string toString();
  virtual NodeFun* get_copy();
  virtual void swap_var(std::string var, Node* tree);
};

/**
 * CoolProp class
 */

class NodePropsSI : public NodeFun{
protected:
  double TMAX,PMAX,TMIN,PMIN;
public:
  NodePropsSI(std::string alias, int number, Node** var);
  NodePropsSI(Node** in, double Tmax, double Pmax, double Tmin, double Pmin);
  virtual double eval(Scope &local);
  virtual NodePropsSI* get_copy();
};


/**
 * Operation Node: + - * / ^
 */
class NodeOp : public NodeFun {
public:
  NodeOp(char symbol, Node* a, Node* b);
  virtual char get_type(){return 'o';}
  virtual double eval(Scope &local);
  virtual std::string toString();
  virtual NodeOp* get_copy();
};

#endif // _NODE_
