#include "solver.hpp" // prototypes
//#include <emscripten.h> // wasm

/* *
 * TO-DO
 * Change ints to unsign or size_t (they are different)
 * Negative guesses (solver options)
 * Verify if jacobian and step are valid
 * Store guesses and promote options that are more distant
 */

/**
 * Variables constructor
 */
Variables::Variables(const std::vector<Node*> &forest, Scope &local){
  std::vector<StringSet> eq;
  StringSet dummy;
  for (const auto &tree : forest){
    dummy = tree->findVars(local);
    eq.push_back(dummy);
    all.insert(dummy.begin(),dummy.end());
  }
  n = forest.size();
  bool *store = new bool[n*n];
  int j = 0;
  for (int i=0; i<n; ++i){      // equation
    j = 0;
    for (const auto &name:all){ // name
      if (eq[i].find(name) != eq[i].end()){
	store[j+i*n] = true;
      } else{
	store[j+i*n] = false;
      }
      ++j;
    }
  }
  table = store;
};

/**
 * Variables copy
 */
Variables::Variables(const Variables &original){
  // To avoid double deletion and memory leaks
  n = original.n;
  all = original.all;
  table = new bool[n*n];
  for (int i=0;i<n;++i){
    for (int j=0;j<n;++j){
      table[j+i*n] = original.table[j+i*n];
    }
  }
}

/**
 * Calculates the numerical derivative
 */
double dfdx(Node* &tree, Scope &guess, std::string name, double y){
  // Set x
  const double rdiff = 1e-8;
  const double x =  guess[name];
  double dx = x*(1+rdiff);
  dx = x==dx ? rdiff : dx; // avoid zero difference
  guess[name] = dx;

  // Get y and dy/dx
  const double dy = tree -> eval(guess);
  const double dfdx = (dy-y)/(dx-x);

  // Set x back
  guess[name] = x;
  return dfdx;
}

/**
 * Evaluates a vector of trees (forest)
 */
void evalForest(const std::vector<Node*> &forest, Scope &guess, mat &answers, mat &side){
  Node** root;
  double left, right, higher;
  for (int i=0; i<answers.rows; ++i){
    root = forest[i]->get_inputs();
    left = root[0]->eval(guess);
    right = root[1]->eval(guess);
    higher = left > right ? left : right;
    side.set(i,0,higher);
    answers.set(i,0,-(left-right)); // minus answer
  }
}

/**
 * Evaluates the Jacobian
 */
void evalJacobian(std::vector<Node*> &forest, Scope &guess,const Variables &vars, mat &jac, mat &answers){
  int j;
  double dummy;
  for (int i=0; i<jac.rows; ++i){      // equation
    j = 0;
    for (const auto &name:vars.all){ // name
      dummy=0;
      if (vars.table[j+i*jac.rows]){
	dummy = dfdx(forest[i],guess,name,-answers.get(i,0)); // correct minus answer
      }
      jac.set(i,j,dummy);
      ++j;
    }
  }
}

/**
 * Broyden
 */
void evalBroyden(mat &jac, mat &dx, mat &df)
{
  // Update factor
  mat aux = df - jac * dx;
  double normSquared=0;
  for (int i = 0; i < dx.rows; ++i)
  {
    normSquared += pow(dx.get(i, 0), 2);
  }
  aux = aux/normSquared;
  
  mat update(jac.rows,jac.columns);
  double plus;
  for (int i=0;i<jac.rows;++i){
    for (int j=0;j<jac.columns;++j){
      plus = aux.get(i,0)*dx.get(j,0);
      update.set(i,j,plus);
    }
  }

  //return ans;
  jac += update;
}

/**
 * Updates a scope with values from a mat
 */
void updateScope(Scope &guess,const Variables &vars, const mat &guessN){
  // Here mat has to be sent as reference, otherwise the code will delete the values.
  int i = 0;
  for (const auto &name:vars.all){
    guess[name] = guessN.get(i,0);
    ++i;
  }
}

/**
 * Sums the error in answers
 */
double evalError(const mat &answers){
  double error = 0;
  for (int i=0; i<answers.rows; ++i){
    error+= std::abs(answers.get(i,0));
  }
  return error;
}

/**
 * Sums relative errors
 */
double evalError(const mat &answers,const mat &side){
  double error = 0;
  double foo;
  for (int i=0; i<answers.rows; ++i){
    foo = side.get(i,0);
    if (foo == 0){
      error+= std::abs(answers.get(i,0));
    } else{
      error+= std::abs(answers.get(i,0)/foo); // std::abs for double
    }
  }
  return error;
}

/**
 * Update scope, evaluate and sum errors
 */
double evalError(const mat &guessN, Variables &vars, std::vector<Node*> &forest, Scope &guessScope){
  // Update	
  updateScope(guessScope, vars, guessN);
  // Calculate
  double error=0;
  for (int i=0; i<guessN.rows; ++i){
    error += abs(forest[i]->eval(guessScope));
    if (!isfinite(error)){
      break;
    }
  }
  return error;
}

// To order errors
bool lessError(Guess first, Guess second){
  return (abs(first.second)<abs(second.second));
}

/**
 * Try values and find good guesses
 */
std::vector<Guess> findGuess(Variables &vars, std::vector<Node*> &forest, Scope &guessScope){
  mat guessN(vars.all.size(),1);
  double val, error;
  double list[8] = {0, 1e-3, 0.1, 1, 10, 200, 1e3, 1e5};
  std::vector<Guess> guessList;
  const short max_tries = 1;//0;
  short count = 0;
  int signal;
  while (guessList.empty() && count < max_tries){
    for (int j=0;j<8;++j){
      // Set value
      for (int i=0;i<guessN.rows;++i){
	srand(time(NULL)+rand()+i);
	signal = rand()%10 > 4 ? 1 : -1;
	val = (1+(rand()%1001)*1e-3*signal/2)*list[j]; // -/+ 10% guess
	guessN.set(i,0,val);
      }
      // Update, evaluate and sum errors
      error = evalError(guessN, vars, forest, guessScope);
      // Pair of guess
      if (isfinite(error)){
	guessList.push_back(Guess(guessN,error));
      }
    }
    ++count;
  }
  if (!guessList.empty()){
    std::sort(guessList.begin(),guessList.end(),lessError);
  }
  //else{
  //  throw std::invalid_argument("no guess @findGuess");
  //}
  return guessList;
}


/**
 * Try n*n values for 2D problems
 * Slower, but more reliable
 */
std::vector<Guess> findGuessPair(Variables &vars, std::vector<Node*> &forest, Scope &guessScope){
  mat guessN(vars.all.size(),1);
  double x,y, error;
  double list[8] = {0, 1e-3, 0.1, 1, 10, 200};
  std::vector<Guess> guessList;
  double higher = std::numeric_limits<double>::infinity();
  const short max_tries = 1;//0;
  short count = 0;
  int signal;
  while (guessList.empty() && count < max_tries){
    for (int j=0;j<8;++j){ 
      srand(time(NULL)+rand()+j);
      signal = rand()%10 > 4 ? 1 : -1;
      x = (1+(rand()%1001)*1e-3*signal/2)*list[j]; // 0 to + 1.000
      guessN.set(0,0,x);
      // Set value
      for (int i=0;i<8;++i){
	srand(time(NULL)+rand()+i);
	signal = rand()%10 > 4 ? 1 : -1;
	y = (1+(rand()%1001)*1e-3*signal/2)*list[i]; // 0 to + 1.000
	guessN.set(1,0,y);
	error = evalError(guessN, vars, forest, guessScope);
	if (isfinite(error) && (guessList.size()<3 || error < higher)){
	  guessList.push_back(Guess(guessN,error));
	  higher = error < higher ? error : higher; 
	}
      }
    }
    ++count;
  }
  if (!guessList.empty()){
    std::sort(guessList.begin(),guessList.end(),lessError);
  }
  //else{
  //  throw std::invalid_argument("no guess @findGuessPair");
  //}
  return guessList;
}

/**
 * Brent method for 1D solution
 */
mat brent(std::string var, Node* tree, Scope &guessScope){
  // Get a bracket interval for guess: common values in problems
  double list[16] =  {1e6, 1e4, 6e3, 390, 323, 273, 200, 140, 1, 1e-2, 0, -1e-2, -1, -1e2, -1e4, -1e6};
  // 390 - (323) - 140 : Temperature limits for HAPropsSI
  
  double a = NAN;
  double b = NAN;
  double fa = INFINITY;
  double fb = INFINITY;
  double error;  
  mat guessN(1,1); 
  // Find a suitable bracket from guess list
  for (int j=0;j<16;++j){
    guessScope[var] = list[j];
    error = tree -> eval(guessScope);
    // Bracket
    if (isfinite(error)){
      if (error > 0){
	b = list[j];
	fb = error;
      } else if (error <0){
	a = list[j];
	fa = error;
      } else{
	// Asnwer found
	guessN.set(0,0,list[j]);
	return guessN;
      }	
    }
    if (isfinite(a) && isfinite(b)){
      break;
    }
  }

  if (!isfinite(a) || !isfinite(b)){
    // Method failed
    guessN.set(0,0,NAN);
    return guessN;
  }
    
  // Brent
  if (abs(fa) < abs(fb)){
    std::swap(a,b);
    std::swap(fa,fb);
  }
  double d,s,fs;
  double c = a;
  double fc = fa;
  bool mflag = true;
  int count = 0;
  const double tol = 1e-6;
  const int max = 100;
  while (abs(fb) > tol && abs(b-a)>tol && count < max){
    ++count;
    if (fa != fc && fb != fc){
      s =  a*fb*fc/(fa-b)/(fa-fc) + b*fa*fc/(fb-fa)/(fb-fc) + c*fa*fb/(fc-fa)/(fc-fb);
    } else{
      s = b - fb*(b-a)/(fb-fa);
    }
    if (!((s >= b && s <= a) || (s <= b && s>=a)) ||
	(mflag && abs(s-b) >= abs(b-c)/2) ||
	(!mflag && abs(s-b) >= abs(c-d)/2) ||
	(mflag && abs(b-c) < tol) ||
	(!mflag && abs(c-d) < tol) ){ 
      s = (a+b)/2;
      mflag = true;
    } else{
      mflag = false;
    }
    guessScope[var] = s;
    fs = tree -> eval(guessScope);
    d = c;
    c = b;
    fc = fb;
    if (fa*fs<0){
      b = s;
      fb = fs;
    } else{
      a = s;
      fa = fs;
    }
    if (abs(fa) < abs(fb)){
      std::swap(a,b);
      std::swap(fa,fb);
    }
  }
  guessN.set(0,0,b);
  return guessN;
}

/**
 * Solver for one dimension problems
 */
bool solve(Node* tree, Scope &guessScope){
  // Var = number
  if(tree->get_op() == '-'){
    Node** inputs = tree->get_inputs();
    char ltype = inputs[0] -> get_type();
    char rtype = inputs[1] -> get_type();
    if (ltype == 'v' &&
	(rtype == 'n' || (inputs[1]->findVars(guessScope)).empty()) ){
      std::string name = inputs[0]->toString();
      guessScope[name] = inputs[1]->eval(guessScope);
      return true;
    } else if (rtype == 'v' &&
	       (ltype == 'n' || (inputs[0]->findVars(guessScope)).empty()) ){
      std::string name = inputs[1]->toString();
      guessScope[name] = inputs[0]->eval(guessScope);
      return true;
    }
  }
  
  // Brent
  StringSet vars = tree -> findVars(guessScope);
  std::string var = *vars.begin();
  mat guess = brent(var,tree,guessScope); // kinda slow, but reliable

  // Error
  if (isnan(guess.get(0,0))){
    //throw std::invalid_argument("brent failed @solve");
    return false;
  }
  
  return true;
}


/**
 * Newton method for multiple dimensions
 */
bool solve(std::vector<Node*> &forest, Scope &guessScope){

  // Variables
  Variables vars(forest,guessScope);
  const unsigned n = vars.all.size();

  // Check size
  if(n != forest.size()){
    throw std::invalid_argument("forest size @solve");
  }

  // Guess
  std::vector<Guess> guessList;
  if (n == 2){
    guessList=findGuessPair(vars,forest,guessScope); // better chances of convergence
  } else{
    guessList=findGuess(vars,forest,guessScope);    
  }
  
  // Guess size
  if (guessList.size()==0){
    return false;
  }
  
  // Matrix
  mat guess(n,1);
  mat answers(n,1);
  mat side(n,1);
  mat jac(n,n);
  mat deltaX(n,1);
  mat deltaF(n,1);
  mat deltaG(n,1);

  // Error doubles
  double error = 1;
  double error_line = 1;
  double error_dx = 1;
  double error_rel = 1;
  double error_change = 1;

  // Counters
  const int max = 200;
  const int max_line = 10;
  const int max_tries = 3;
  int count = 0;
  int count_line = 0;

  // Flags and control
  bool computed;
  bool useBroyden;
  bool nostep;
  const double safeStep = 0.8;
  
  for (unsigned g=0; g<guessList.size() && g<max_tries; ++g){
    count = 0;
    error = 1;
    error_dx = 1;
    error_rel = 1;
    error_change = 1;
    useBroyden = false; // flag to use Broyden method
    computed = false;   // flag to indicate if its the calculated Jacobian
    nostep = false;
    
    // Fist evaluation
    guess = guessList[g].first;
    updateScope(guessScope,vars,guess);
    evalForest(forest,guessScope,answers,side);
    error = evalError(answers);
    
    // Newton method: create function to eval convergence
    while (error_dx > 5e-6 && (error_rel > 1e-3 || error > 1e-6) &&
	   count < max){

      if (useBroyden){
	evalBroyden(jac, deltaG, deltaF);
	computed = false;
      } else{
	evalJacobian(forest,guessScope,vars,jac,answers);
	useBroyden = true;
	computed = true;
      }

      // Store values for Broyden method
      deltaG = mat(guess);
      deltaF = mat(answers);

      // Update guess
      deltaX = gaussElimination(jac,answers);
      
      // Limits the update - use just for the first iteration
      for (unsigned i=0;i<n;++i){
	double value = deltaX.get(i,0);
	// Check if step is a finite number
	if (!isfinite(value)){
	  nostep = true;
	  break;
	}
	// POSSIBLE UNSTABLE --> Reduce step error is too high
	if (error>1E3){
	  double reference = guess.get(i,0);
	  if (abs(value/reference)>safeStep){
	    value = value>0 ? reference*safeStep : -reference*safeStep;
	    deltaX.set(i,0,value);
	  }
	}
      }
      if (nostep){
	count=max;
	break;
      }

      guess += deltaX; // Max step
      
      // Line-search loop [Most time is expended here]
      count_line = 0;
      do {
	updateScope(guessScope,vars,guess);
	evalForest(forest,guessScope,answers,side);
	error_line = evalError(answers);
	++count_line;
	// Reduce the step if necessary
	error_change = abs(1-error_line/error);
	if(error_line > error || !isfinite(error_line)){
	  if (computed){
	    guess -= deltaX*(1/pow(2,count_line));
	  } else{
	    useBroyden=false;
	    break;
	  }
	}
      } while ((error_line > error || !isfinite(error_line)) &&
	       count_line < max_line);

      // Try again with Jacobian since Broyden has failed
      if (!computed && !useBroyden){
	continue;
      }

      // Required for the Broyden method
      deltaG = guess-deltaG;
      deltaF -= answers; // -answer bug
      
      if(count_line == max_line && count != 0){
	// Line-search failed - break
	count = max;
       	break;
      }
      
      // Convergence conditions
      error = error_line;
      error_rel = evalError(answers,side);
      error_dx = count !=0 ? evalError(deltaX,guess) : 1;

      // Check error change <- break earlier
      if (!isfinite(error) || error_change < 1e-3){
	count = max;
	break;
      }    

      ++count;
    }

    // Break guess test loop
    if (count != max){
      break;
    }
  }
      
  if(count == max){
    //throw std::invalid_argument("not converged @solve");
    return false;
  }

  return true;
}
