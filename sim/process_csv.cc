#include "common.hh"
#include "process_csv.hh"

std::vector< std::vector <sim::real> >
sim::convert_csv_strings_to_reals(std::vector< std::vector <std::string> >&
				  matrix_strings)
{
  std::vector< std::vector <sim::real> > matrix_real;

  // Skip the first row which is the header
  for (size_t i = 1; i < matrix_strings.size(); ++i) {
    if (matrix_strings[i].size() != matrix_strings[0].size())
      throw sim::SimulationException("Csv rows must have same number entries.");
    std::vector<sim::real> row;
    for (size_t j = 0; j < matrix_strings[i].size(); ++j) {
      row.push_back(atof(matrix_strings[i][j].c_str()));
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

  if (csv_in.fail())
    throw SimulationException("Can't open csv file.");

  while (std::getline(csv_in, s)) {
    lines.push_back(s);
  }
  if (csv_in.eof() == false && csv_in.fail() == true)
    throw SimulationException("Error reading csv file.");

  csv_matrix = process_csv_lines(lines, delimiter);
  return csv_matrix;
}
