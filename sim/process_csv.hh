#ifndef PROCESS_CSV_H
#define PROCESS_CSV_H


#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "sim/common.hh"

namespace sim {
  std::vector< std::vector <std::string> >
  process_csv_file(const char *filename, const char delimiter=',');
}

#endif
