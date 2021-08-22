#include "text.hpp"  // function prototypes

/**
 * Turn equations into expressions
 * lhs = rhs -> lhs-(rhs) 
 */
std::string minusExp(std::string equation){
  const std::size_t equal = equation.find('=');
  if (equal == std::string::npos){
    throw std::invalid_argument("equation without '=' @minusExp");
  }
  const std::string exp = equation.substr(0,equal)+'-'+'('+equation.substr(equal+1)+')';
  return exp;
}

/**
 * Break sublines and remove comments
 */
std::string breakLines(std::string &text){
  const unsigned semicolon = text.find(';');
  const unsigned hash = text.find('#');
  std::string line;
  if (semicolon < hash){
    line = text.substr(0,semicolon);
    text = semicolon+1 < text.length() ? text.substr(semicolon+1): text.substr(semicolon);
  } else if (hash < semicolon){
    line = text.substr(0,hash);
    text.clear();
  } else{
    line = text;
    text.clear();
  }

  // Check if line is whitespace
  if(line.find_first_not_of(' ') == std::string::npos){
    line.clear();
  }
  return line;
}

/**
 * Get lines from a file
 */
std::vector<std::string> getLines(std::string filename){
  std::ifstream file(filename);
  std::vector<std::string> lines;
  std::string line, subline;
  while (std::getline(file,line)){
    while (!line.empty()){
      subline = breakLines(line);
      if (!subline.empty()){
	lines.push_back(minusExp(subline));
      }
    }
  }
  file.close();
  return lines;
}
