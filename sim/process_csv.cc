#include <cerrno>
#include <climits>
#include <cfloat>
#include <cstring>
#include <sstream>

#include "common.hh"
#include "process_csv.hh"

/* Convert C string argument to unsigned. */
unsigned
sim::strtou(const char *str)
{
  char *endptr;
  long val;
  std::stringstream msg;

  errno = 0;    /* To distinguish success/failure after call */
  val = strtol(str, &endptr, 10);

  /* Check for various possible errors */

  if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN))
      || (errno != 0 && val == 0)) {
    throw ArgException(strerror(errno));
  }

  if (strcmp(endptr, "") != 0) {
    msg << "Number " << str << " is not valid. " << std::endl;
    throw ArgException(msg.str().c_str());
  }

  if (endptr == str) {
    msg << "No digits were found in number" << std::endl;
    throw ArgException(msg.str().c_str());
  }

  if (val < 0) {
    msg << "Number " << str << " smaller than 0. "
	<< "Must be unsigned integer." << std::endl;
    throw ArgException(msg.str().c_str());
  }

  if (val > UINT_MAX) {
    msg << "Number " << str << " too large. "
	<< "Must be unsigned integer <= " << UINT_MAX << std::endl;
    throw ArgException(msg.str().c_str());
  }

  return (unsigned) val;
}

/* Convert C string argument to double. */
double
sim::strtor(const char *str)
{
  char *endptr;
  double val;
  std::stringstream msg;

  errno = 0;    /* To distinguish success/failure after call */
  val = strtod(str, &endptr);

  /* Check for various possible errors */

  if (errno) {
    throw ArgException(strerror(errno));
  }

  if (strcmp(endptr, "") != 0) {
    msg << "Number " << str << " is not valid. " << std::endl;
    throw ArgException(msg.str().c_str());
  }

  if (endptr == str) {
    msg << "No digits were found in number" << std::endl;
    throw ArgException(msg.str().c_str());
  }

  return val;
}

std::vector< std::vector <sim::real> >
sim::convert_csv_strings_to_reals(std::vector< std::vector
				  <std::string> >& matrix_strings,
				  const bool true_matrix)
{
  std::vector< std::vector <sim::real> > matrix_real;

  // Skip the first row which is the header
  for (size_t i = 1; i < matrix_strings.size(); ++i) {
    if (true_matrix && matrix_strings[i].size() != matrix_strings[0].size())
      throw sim::SimulationException("CSV rows must have same number entries.");
    std::vector<sim::real> row;
    for (auto & s : matrix_strings[i]) {
      if (s == "")
	row.push_back(0.0);
      else
	row.push_back(strtor(s.c_str()));
    }
    matrix_real.push_back(row);
  }
  return matrix_real;
}

static std::vector< std::vector <std::string> >
process_csv_lines(const std::vector<std::string> & lines,
		       const char delim)
{
  std::vector< std::vector <std::string> >  csv_matrix;
  for (auto & line : lines) {
    bool inquote = false;
    std::vector <std::string> csv_line;
    std::string csv_entry = "";
    for (auto & c : line) {
      if (c == delim && inquote == false) {
	csv_line.push_back(csv_entry);
	csv_entry = "";
      } else {
	if (c == '"')
	  inquote = !inquote;
	csv_entry = csv_entry + c;
      }
    }
    if (csv_entry != "") {
      csv_line.push_back(csv_entry);
    }
    csv_matrix.push_back(csv_line);
  }
  return csv_matrix;
}

std::vector< std::vector <std::string> >
sim::process_csv_file(const char *filename, const char delimiter)
{
  std::ifstream csv_in(filename);
  std::string s;
  std::vector<std::string> lines;
  std::vector< std::vector <std::string> >  csv_matrix;

  if (csv_in.fail()) {
    std::stringstream ss;
    ss << "Can't open CSV file " << filename;
    throw SimulationException(ss.str().c_str());
  }

  while (std::getline(csv_in, s)) {
    lines.push_back(s);
  }
  if (csv_in.eof() == false && csv_in.fail() == true)
    throw SimulationException("Error reading csv file.");

  csv_matrix = process_csv_lines(lines, delimiter);
  return csv_matrix;
}
