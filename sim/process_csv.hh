#ifndef PROCESS_CSV_H
#define PROCESS_CSV_H


#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "sim/common.hh"

namespace sim {
  unsigned strtou(const char *str);
  double strtor(const char *str);
  std::vector< std::vector <std::string> >
  process_csv_file(const char *filename, const char delimiter=',');
  std::vector< std::vector <real> >
  convert_csv_strings_to_reals(std::vector< std::vector <std::string> >& m,
			       const bool true_matrix = true);
}

#endif
