#include "matrix.hpp"
//#include <emscripten.h> // wasm

/**
 * mat construtor
 * builds a zero matrix: (n x m) -> [0]nxm
 */
mat::mat(int r, int c){
  rows=r;
  columns=c;
  double* store=new double[r*c];
  for (int i=0; i<(r*c); ++i){
    store[i]=0;
  }
  eArray=store;
}

/**
 * mat constructor
 * builds from existing array
 */
mat::mat(int r, int c, double* elem){
  rows = r;
  columns = c;
  double* store=new double[r*c];
  for (int i=0; i<(r*c); ++i){
    store[i]=elem[i];
  }
  eArray=store;
} 

/**
 * mat destructor
 */  
mat::~mat(){
  delete[] eArray;
}

/**
 * copy
 * has a copy of elements (if necessary)
 */
mat::mat(const mat &original){
  rows = original.rows;
  columns = original.columns;
  eArray = new double[rows*columns];
  for (int i=0;i<rows;++i){
    for (int j=0;j<columns;++j){
      eArray[j+i*columns] = original.get(i,j);
    }
  }
}

/**
 * mat operator +=
 */
mat mat::operator+= (mat& other){
  double foo, bar;
  for (int i=0; i<rows; ++i){
    for (int j=0; j<columns; j++){
      foo=this->get(i,j);
      bar=other.get(i,j);
      this->set(i,j,foo+bar);
    }
  }
  return *this;
}

/**
 * mat operator -=
 */
mat mat::operator-= (mat& other){
  double foo, bar;
  for (int i=0; i<rows; ++i){
    for (int j=0; j<columns; j++){
      foo=this->get(i,j);
      bar=other.get(i,j);
      this->set(i,j,foo-bar);
    }
  }
  return *this;
}

/**
 * mat operator =
 * here the pointer is moved to avoid excessive copies and double deletion
 */
mat& mat::operator=(mat other){
  rows = std::move(other.rows);
  columns = std::move(other.columns);

  double foo;
  for (int i=0; i<rows; ++i){
    for (int j=0; j<columns; j++){
      foo=other.get(i,j);
      this->set(i,j,foo);
    }
  }
  
  //delete[] eArray; // avoid memory leaks
  //eArray = std::exchange(other.eArray,nullptr);
  return *this;
}

// /**
//  * mat operator *
//  * for scalar multiplication
//  */
// mat& mat::operator*(double scalar){
//   double foo;
//   for (int i=0; i<rows; ++i){
//     for (int j=0; j<columns; j++){
//       foo=this->get(i,j);
//       this->set(i,j,foo*scalar);
//     }
//   }
//   return *this;
// }


// /**
//  * mat operator /
//  * for scalar division
//  */
// mat& mat::operator/(double scalar){
//   double foo;
//   for (int i=0; i<rows; ++i){
//     for (int j=0; j<columns; j++){
//       foo=this->get(i,j);
//       this->set(i,j,foo/scalar);
//     }
//   }
//   return *this;
// }

/**
 * mat operator *
 * for matrix
 */
mat mat::operator* (mat other){
  double sum;
  mat answer(rows,other.columns);
  for (int i=0; i<rows; ++i){
    for (int j=0; j<other.columns;++j){
      sum=0;
      for (int k=0; k<columns;++k){
	sum+=get(i,k)*other.get(k,j);
      }
      answer.set(i,j,sum);
    }
  }
  return answer;
}

/**
 * mat operator -
 */
mat mat::operator- (mat other){
  double sum;
  mat answer(rows,columns);
  for (int i=0; i<rows; ++i){
    for (int j=0; j<columns;++j){
      sum = get(i,j)-other.get(i,j);
      answer.set(i,j,sum);
    }
  }
  return answer;
}

/**
 * Swaps the rows of a matrix
 */
void swapRow(mat& matrix,const int rowA,const int rowB){
  const int c = matrix.columns;
  for (int i=0; i<c; ++i){
    std::swap(matrix.eArray[i+rowA*c],matrix.eArray[i+rowB*c]);
  }  
}

/**
 * Solver for simple linear systems: 
 * Takes [coeff]mxn and [equals]mx1 and solve it;
 */
mat gaussElimination(mat& coeff, mat& equals){  
  double first,factor,aux;
  const int rows = coeff.rows;
  const int columns = coeff.columns;

  // Elimination - Linear combination of rows
  for (int pivot=0; pivot<rows; ++pivot){
    // if is zero, change for another row unless all rows bellow are zero
    if (coeff.get(pivot,pivot) == 0) {
      for (int i=pivot+1; i<rows; ++i){
	if (coeff.get(i,pivot)!=0){ // check line below
	  swapRow(coeff,pivot,i);
	  swapRow(equals,pivot,i);
	  break;
	}  
      }
    }
    for (int obj=pivot+1; obj<rows; ++obj){
      first=coeff.get(obj,pivot);
      if (first!=0){
   	// Calculate factor
   	factor=first/coeff.get(pivot,pivot);
   	// Update row
   	for (int elem=pivot;elem<columns;++elem){
	  // Could be skipped the first element (could be confusing)
	  aux=coeff.get(obj,elem)-factor*coeff.get(pivot,elem);
	  coeff.set(obj,elem,aux);
	}
	// Update the equal matrix
	aux=equals.get(obj,0)-factor*equals.get(pivot,0);
	equals.set(obj,0,aux);
      }
    }
  }

  // Answer - Backward substitution
  mat answer(rows,1);
  double value;
  for (int back=rows-1;back>=0;--back){
    aux=0;
    if (back<rows){
      for (int elem=back;elem<columns;++elem){
    	aux+=coeff.get(back,elem)*answer.get(elem,0);
      }
    }
    value=(equals.get(back,0)-aux)/coeff.get(back,back);
    answer.set(back,0,value);
  }
  return answer;
}
