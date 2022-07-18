#include "solver.hpp" // prototypes
#include <random>
//#include <emscripten.h> // wasm

/* *
 * TO-DO
 * Negative guesses (solver options)
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
  unsigned j = 0;
  for (unsigned i=0; i<n; ++i){      // equation
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
  for (unsigned i=0;i<n;++i){
    for (unsigned j=0;j<n;++j){
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
  double dx;
  dx = x==0 ? rdiff : x*(1+rdiff); // avoid zero difference

  // Get y and dy/dx
  guess[name] = dx;
  double dy = tree -> eval(guess);
  double dfdx = (dy-y)/(dx-x);
  
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
  for (unsigned i=0; i<answers.rows; ++i){
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
  unsigned j;
  double dummy;
  for (unsigned i=0; i<jac.rows; ++i){      // equation
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
  for (unsigned i = 0; i < dx.rows; ++i)
  {
    normSquared += pow(dx.get(i, 0), 2);
  }
  for (unsigned i = 0; i < aux.rows ; ++i){
    aux.set(i,0,aux.get(i,0)/normSquared);
  }
  
  mat update(jac.rows,jac.columns);
  double plus;
  for (unsigned i=0;i<jac.rows;++i){
    for (unsigned j=0;j<jac.columns;++j){
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
  unsigned i = 0;
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
  for (unsigned i=0; i<answers.rows; ++i){
    error+= pow(answers.get(i,0),2);
  }
  return error;
}

/**
 * Sums relative errors
 */
double evalError(const mat &answers,const mat &side){
  double error = 0;
  double foo;
  for (unsigned i=0; i<answers.rows; ++i){
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
  for (unsigned i=0; i<guessN.rows; ++i){
    error += pow(forest[i]->eval(guessScope),2);
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
std::vector<Guess> findGuess(Variables &vars, std::vector<Node*> &forest, Scope &guessScope, unsigned i){
  mat guessN(vars.all.size(),1);
  double val, error;
  double list[8] = {0, 1e-3, 0.1, 1, 10, 200, 1e3, 1e5};
  std::vector<Guess> guessList;
  const short max_tries = 1;//0;
  short count = 0;
  int signal;
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count()+i*100;
  std::default_random_engine generator(seed);
  std::uniform_real_distribution<double> distribution(0.0,1.0);

  while (guessList.empty() && count < max_tries){
    for (unsigned j=0;j<8;++j){
      // Set value
      for (unsigned i=0;i<guessN.rows;++i){
	signal = distribution(generator) > 0.5 ? 1 : -1;
	val = (1+distribution(generator)*signal/2)*list[j]; // -/+ 10% guess
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
std::vector<Guess> findGuessPair(Variables &vars, std::vector<Node*> &forest, Scope &guessScope, unsigned i){
  mat guessN(vars.all.size(),1);
  double x,y, error;
  double list[8] = {0, 1e-3, 0.1, 1, 10, 200, 1e3, 1e5};
  std::vector<Guess> guessList;
  const short max_tries = 1;//0;
  short count = 0;
  int signal;
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count()+100*i;
  std::default_random_engine generator(seed);
  std::uniform_real_distribution<double> distribution(0.0,1.0);

  while (guessList.empty() && count < max_tries){
    for (unsigned j=0;j<8;++j){ 
      signal = distribution(generator) > 0.5 ? 1 : -1;
      x = (1+distribution(generator)*signal/2)*list[j]; // 0 to + 1.000
      guessN.set(0,0,x);
      // Set value
      for (unsigned i=0;i<8;++i){
	signal = distribution(generator) > 0.5 ? 1 : -1;
	y = (1+distribution(generator)*signal/2)*list[i]; // 0 to + 1.000
	guessN.set(1,0,y);
	error = evalError(guessN, vars, forest, guessScope);
	if (isfinite(error)){
	  guessList.push_back(Guess(guessN,error)); 
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
  for (unsigned j=0;j<16;++j){
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
  unsigned count = 0;
  const double tol = 1e-6;
  const short max = 100;
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
 *Norm
 */
double norm(mat &vector){
  double ans = 0;
  for (unsigned i=0;i<vector.columns;++i){
    ans += pow(vector.get(i,0),2);
  }
  return sqrt(ans);
}


/**
 * Newton method for multiple dimensions
 */
bool solve(std::vector<Node*> &forest, Scope &guessScope, unsigned i){

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
    guessList=findGuessPair(vars,forest,guessScope,i); // better chances of convergence
  } else{
    guessList=findGuess(vars,forest,guessScope,i);    
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

  // Counters
  const short max = 200;
  const short max_line = 10;
  const short max_tries = 999; // try everything
  short count = 0;
  short count_line = 0;

  // Flags and control
  bool computed;
  bool useBroyden;
  bool nostep;

  // Statistics
  unsigned evals = 0;
  unsigned levals = 0;

  // Line search
  double lambda = 1;
  double lambda_pre = 1;
  
  for (unsigned g=0; g<guessList.size() && g<max_tries; ++g){
    //std::cout << "new try" << std::endl;
    //std::cout << guessList[g].second << std::endl;
    count = 0;
    error = 1;
    error_dx = 1;
    error_rel = 1;
    useBroyden = false; // flag to use Broyden method
    computed = false;   // flag to indicate if its the calculated Jacobian
    nostep = false;
    
    // Fist evaluation
    guess = guessList[g].first;
    updateScope(guessScope,vars,guess);
    evalForest(forest,guessScope,answers,side);
    error = evalError(answers);
    evals +=1 ;
    
    // Newton method: create function to eval convergence
    while (error_dx > 1e-7 && (error_rel > 1e-3 || sqrt(error) > 1e-5) &&
	   count < max){
      
      
      if (useBroyden){
	evalBroyden(jac, deltaG, deltaF);
	computed = false;
      } else{
	evalJacobian(forest,guessScope,vars,jac,answers);
	evals+=1;
	useBroyden = true;
	computed = true;
      }

      // Check if jacobian is valid
      for (unsigned i=0;i<n;++i){
	//std::cout << guess.get(i,0) << " ";
	for (unsigned j=0;j<n;++j){
	  if (!isfinite(jac.get(i,j))){
	    nostep = true;
	    break;
	  }
	}
	if (nostep){
	  break;
	}
      }
      if (nostep){
	count=max;
	break;
      }
      //std::cout << std::endl;

      // Store values for Broyden method
      deltaG = mat(guess);
      deltaF = mat(answers);

      // Update guess
      deltaX = gaussElimination(jac,answers);
      // Limits the update - use just for the first iteration
      for (unsigned i=0;i<n;++i){
	// Check if step is a finite number
	if (!isfinite(deltaX.get(i,0))){
	  nostep = true;
	  break;
	}
	// Break loop
	if (nostep){
	  break;
	}
      }
      if (nostep){
	count=max;
	break;
      }

      // Check max step
      double guessNorm = norm(guess);
      double stepNorm = norm(deltaX);
      if (stepNorm > guessNorm*1E3){
      	for (unsigned i = 0; i<n; ++i){
      	  deltaX.set(i,0,deltaX.get(i,0)*guessNorm*1E3/stepNorm);
      	}
      }
      guess += deltaX; // Max step
      
      // Line-search loop [Most time is expended here]
      count_line = 0;
      lambda = 1;
      lambda_pre = 1;
      do {
	updateScope(guessScope,vars,guess);
	evalForest(forest,guessScope,answers,side);
	levals +=1;
	error_line = evalError(answers);
	++count_line;
	// Update guess
	if (error_line > error || !isfinite(error_line)){
	  // Make it closer if too bad, otherwise be optimistic
	  lambda = !isfinite(lambda) ? 0.1 : 0.5;
	  for (unsigned i = 0; i<n ; ++i){
	    guess.set(i,0,guess.get(i,0)-deltaX.get(i,0)*lambda_pre*(1-lambda));
	  }
	  lambda_pre *= lambda;
	}
      } while ((error_line > error || !isfinite(error_line)) &&
	       count_line < max_line && lambda_pre > 1E-3);
      
      // Try again with Jacobian since Broyden has failed
      if (!computed){
	useBroyden = false;
	continue;
      } else {
	if(count_line == max_line && count != 0){
	  // Line-search failed - break
	  count = max;
	  break;
	}
      }

      // Required for the Broyden method
      deltaG = guess-deltaG;
      deltaF -= answers; // -answer bug
      
      // Convergence conditions
      error = error_line;
      error_rel = evalError(answers,side);
      error_dx = count !=0 ? evalError(deltaX,guess) : 1;    

      // Check error change <- break earlier
      if (!isfinite(error)){
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
    //std::cout << "Failure: " << evals << " evals " << levals << " levals" << std::endl;
    //throw std::invalid_argument("not converged @solve");
    return false;
  }
  
  //std::cout << "Sucess: " << evals << " evals " << levals << " levals" << std::endl;

  return true;
}
