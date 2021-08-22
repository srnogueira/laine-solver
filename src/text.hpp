#ifndef _TEXT_
#define _TEXT_

#include <string>    // string
#include <vector>    // vector
#include <fstream>   // in-out files

std::string minusExp(std::string equation);
std::string breakLines(std::string &text);
std::vector<std::string> getLines(std::string filename);

#endif
