#ifndef _MATRIX_
#define _MATRIX_

#include <utility>
#include <algorithm>

/**
 * Matrix struct
 */
struct mat{
  // Constructors
  mat(int r, int c);
  mat(int r, int c, double* elem);
  ~mat();
  mat(const mat &original);
  // Data
  int rows;
  int columns;
  double* eArray;
  // Methods
  double get(int r, int c) const {return eArray[c+r*columns];}
  void set(int r, int c, double v){eArray[c+r*columns]=v;}
  // Overloaded operators
  mat& operator* (double scalar);
  mat& operator/ (double scalar);
  mat operator* (mat other);
  mat operator- (mat other);
  mat operator+= (mat& other);
  mat operator-= (mat& other);
  mat& operator=(mat other);
};

// mat functions
void swapRow(mat& matrix,const int rowA,const int rowB);
mat gaussElimination(mat& coeff, mat& equals);

#endif

