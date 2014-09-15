#include <algorithm>
#include <iostream>
#include <cstring>

#include "sim.hh"
#include "Simulation.hh"
#include "process_csv.hh"

using namespace sim;

namespace sim {
  unsigned thread_num  = 0;
  thread_local std::mt19937_64 rng;
}

Simulation::
#ifdef SIM_VECTORIZE
Simulation(unsigned seed,
	   size_t num_parms,
	   size_t num_states)
#else
Simulation(unsigned seed)
#endif
: seed_(seed + sim::thread_num)
{
#ifdef SIM_VECTORIZE
  num_parms_ = num_parms;
  parameters.resize(num_parms_);
  num_states_ = num_states;
  states.resize(num_states_);
#else
#endif
}

Simulation::~Simulation()
{
  for (auto & agent : agents)
    delete agent;
  for (auto & agent : dead_agents)
    delete agent;
}


Agent*
Simulation::append_agent()
{
  Agent *a = new Agent(agent_count_++);
#ifdef SIM_VECTORIZE
  a->states.resize(num_states_);
#endif
  agents.push_back(a);
  return a;
}

void
Simulation::set_number_agents(const unsigned num_agents)
{
  for (unsigned i = 0; i < num_agents; ++i)
    append_agent();
}

unsigned
Simulation::iteration() const
{
  return iteration_;
}


void
Simulation::set_parameter(const unsigned parameter,
			  const std::initializer_list<real> values,
			  const char *name)
{
  parameters[parameter] = values;

  if (name) {
    parms_names[parameter] = std::string(name);
    names_parms[std::string(name)] = parameter;
  }
}

void
Simulation::set_parameters(const std::initializer_list< const parameter_parms_>
			   parms)
{
  for (auto & p: parms)
    set_parameter(p.parameter, p.values, p.name);
}

void
Simulation::set_parameter_name(const unsigned parameter,
			       const char *name)
{
  parms_names[parameter] = std::string(name);
  names_parms[std::string(name)] = parameter;
}

void
Simulation::set_parameter_names(std::initializer_list< std::pair <
				const unsigned,
				const char * > > parameter_names)
{
  for (auto & s : parameter_names)
    set_parameter_name(s.first, s.second);
}


void
Simulation::set_state_name(const unsigned state,
			   const char *name)
{
  states_names[state] = std::string(name);
  names_states[std::string(name)] = state;
}

void
Simulation::set_state_names(std::initializer_list< std::pair <
			    const unsigned, const char * > > state_names)
{
  for (auto & s : state_names)
    set_state_name(s.first, s.second);
}

void
Simulation::set_global_state_initializers(const std::initializer_list
					  <GlobalStateInit>  init_funcs)
{
  init_global_state_funcs_ = init_funcs;
}


void
Simulation::set_global_states()
{
  for (auto & f : init_global_state_funcs_)
    f(this);
}

void
Simulation::set_global_states(const std::initializer_list<GlobalStateInit>
			      init_funcs)
{
  set_global_state_initializers(init_funcs);
  set_global_states();
}

void
Simulation::set_agent_initializers(const std::initializer_list <AgentInit>
				   init_funcs)
{
  init_agent_funcs_ = init_funcs;
}

void
Simulation::set_global_events(const std::initializer_list<GlobalEvent> evnts) {
  global_events = evnts;
}


void
Simulation::kill_agent(size_t agent_index)
{
  dead_agents.push_back(agents[agent_index]);
  agents[agent_index] = agents.back();
  agents.pop_back();
}

void
Simulation::kill_agent()
{
  kill_agent(current_agent_index_);
}


void
Simulation::set_agent_states()
{
  iteration_ = 0;
  for (auto & agent : agents)
    for (auto & init_func : init_agent_funcs_)
      init_func(agent, this);
}

void
Simulation::set_agent_states(const std::initializer_list <
			     std::function<void(Agent *,
						Simulation *)>> init_funcs)
{
  set_agent_initializers(init_funcs);
  set_agent_states();
}

void
Simulation::set_events(const std::initializer_list<AgentEvent> events)
{
  agent_events = events;
}

void
Simulation::set_reports(const std::initializer_list <report_parms_> reprts)
{
  for (auto & r : reprts)
    reports.push_back(Report(r.report_func, r.iteration, r.before, r.after));
}

void
Simulation::perturb_parameters(const Perturbers& perturbers)
{
  for (auto & perturber : perturbers) {
    auto & vals = parameters[perturber.first];
    for (size_t i = 0; i < vals.size(); ++i)
      vals[i] = savedParameters_[perturber.first][i] + perturber.second(rng);
  }
}


void
Simulation::montecarlo(const unsigned num_steps,
		       const bool interim_reports,
		       const Perturbers& perturbers,
		       std::function<bool(const Simulation *,
					  unsigned)> carryon)
{
  try {
#ifdef SIM_VECTORIZE
    savedParameters_.resize(num_parms_);
#endif
    // Save parameters
    for (auto & perturber : perturbers)
      std::copy(parameters[perturber.first].begin(),
		parameters[perturber.first].end(),
		std::back_inserter(savedParameters_[perturber.first]));
    // Run the simulations
    for (int i = 0; carryon(this, i); ++i) {
      perturb_parameters(perturbers);
      simulate(num_steps, interim_reports);
    }
    // Restore the parameters
    for (auto & perturber : perturbers)
      std::copy(savedParameters_[perturber.first].begin(),
		savedParameters_[perturber.first].end(),
		parameters[perturber.first].begin());
  } catch(std::exception &e) {
    throw SimulationException(e.what());
  }
}

void
Simulation::initialize_states()
{
  set_global_states();
  if (csv_agent_matrix_.size())
    set_agents_from_csv();
  set_agent_states();
}

void
Simulation::simulate(unsigned num_steps,
		     bool interim_reports)
{
  try {
    initialize_states();
    // Reports at beginning
    for (auto & report : reports)
      if (report.before())
	try {
	  report(this);
	} catch (std::exception &e) {
	  std::cerr << "Exception processing pre-simulation report "
		    << __FILE__ << " " << __LINE__ << std::endl;
	  std::cerr << "Report address: " << &report << std::endl;
	  throw SimulationException(e.what());
	}
    // Simulate
    unsigned iterations = num_steps;
    for (; iteration_ < iterations; ++iteration_) {
      // Global events
      for (const auto & event : global_events)
	try {
	  event(this);
	} catch (std::exception &e) {
	  std::cerr << "Exception processing global event "
		    << __FILE__ << " " << __LINE__ << std::endl;
	  std::cerr << "Iteration: " << iteration_ << std::endl;
	  std::cerr << "Event address: " << &event << std::endl;
	  throw SimulationException(e.what());
	}
      std::shuffle(agents.begin(), agents.end(), rng);
      current_agent_index_ = 0;
      for (auto & agent : agents) {
	for (auto & event : agent_events)
	  try {
	    event(this, agent);
	  } catch  (std::exception &e) {
	    std::cerr << "Exception processing agent event "
		      << __FILE__ << " " << __LINE__ << std::endl;
	    std::cerr << "Iteration: " << iteration_ << std::endl;
	    std::cerr << "Agent id: " << agent->id() << std::endl;
	    std::cerr << "Agent index: " << current_agent_index_ << std::endl;
	    std::cerr << "Event address: " << &event << std::endl;
	    throw SimulationException(e.what());
	  }
	++current_agent_index_;
      }
      if (interim_reports) {
	for (auto & report : reports) {
	  try {
	    unsigned freq = report.frequency();
	    if ( freq && ( (iteration_ + 1) % freq == 0 ))
	      report(this);
	  } catch (std::exception &e) {
	    std::cerr << "Exception processing end of simulation report "
		      << __FILE__ << " " << __LINE__ << std::endl;
	    std::cerr << "Iteration: " << iteration_ << std::endl;
	    std::cerr << "Report address: " << &report << std::endl;
	    throw SimulationException(e.what());
	  }
	}
      }
    }
    // Reports at end
    for (auto & report : reports)
      if (report.after())
	try {
	  report(this);
	} catch (std::exception &e) {
	  std::cerr << "Exception processing final report "
		    << __FILE__ << " " << __LINE__ << std::endl;
	  std::cerr << "Report address: " << &report << std::endl;
	  throw SimulationException(e.what());
	}
  } catch (std::exception &e) {
    std::cerr << "Exception at " << __FILE__ << " " << __LINE__ << std::endl;
    throw SimulationException(e.what());
  }
}

/* If an event occurs with probability P1 in time T1,
   then the probability, P2, of it occuring in time T2 is:
   P2 = 1 - (1 - P1)^(T1/T2).
   This function returns P2
*/

real
Simulation::prob_event(real prob,
		       real prob_time_period,
		       real actual_time_period) const
{
  return 1 - pow((1 - prob), (actual_time_period / prob_time_period));
}

real
Simulation::prob_event(unsigned parameter) const
{
  return prob_event(parameters.at(parameter)[0],
		    1.0, parameters.at(TIME_STEP_SIZE_PARM)[0]);
}

bool
Simulation::is_event(real rand,
		     real prob,
		     real prob_time_period,
		     real actual_time_period) const
{
  if (rand < prob_event(prob, prob_time_period, actual_time_period))
    return true;
  else
    return false;
}

bool
Simulation::is_event(real prob,
		     real prob_time_period,
		     real actual_time_period) const
{
  std::uniform_real_distribution<> uni_dis;
  return is_event(uni_dis(rng), prob, prob_time_period, actual_time_period);
}


bool
Simulation::is_event(unsigned parameter) const
{
  return is_event(parameters.at(parameter)[0], 1.0,
		  parameters.at(TIME_STEP_SIZE_PARM)[0]);
}

void
Simulation::set_parameters_from_csv()
{

}

void
Simulation::set_agents_from_csv()
{
  for (size_t i = 0; i < csv_agent_matrix_.size(); ++i) {
    unsigned num_agents = (unsigned)
      csv_agent_matrix_[i][csv_num_agents_col_];
    for (size_t j = 0; j < num_agents; ++j) {
      Agent *a = append_agent();
      for (size_t k = 0; k < csv_agent_matrix_[i].size(); ++k) {
	if (k == csv_num_agents_col_)
	  continue;
	a->states[names_states[csv_agent_col_headings_[k]]] =
	  {csv_agent_matrix_[i][k]};
      }
    }
  }
}


void
Simulation::set_parameters_csv_initializer(const char *filename, char delim)
{
  std::vector <std::vector <std::string> > csv_matrix_strings;
  std::vector <std::string> csv_parameter_col_headings;
  try {
    csv_matrix_strings = process_csv_file(filename, delim);
    if (csv_matrix_strings.size() == 0)
      throw SimulationException("No headings in CSV file.");

    // Process header
    csv_parameter_col_headings = csv_matrix_strings[0];
    for (size_t i = 0; i < csv_parameter_col_headings.size(); ++i) {
      if (names_parms.find(csv_parameter_col_headings[i]) == names_parms.end())
	throw SimulationException("CSV key not found in parameters table");
    }

    // Fill each of the parameter vectors
    for (size_t i = 1; i < csv_matrix_strings.size(); ++i) {
      for (size_t j = 0; j < csv_matrix_strings[i].size(); ++j) {
	if (csv_matrix_strings[i][j] != "") {
	  double d = strtor(csv_matrix_strings[i][j].c_str());
	  parameters[names_parms.at(csv_parameter_col_headings[j])].
	    push_back(d);
	}
      }
    }
  } catch (std::exception &e) {
    throw SimulationException(e.what());
  }
}


void
Simulation::set_agent_csv_initializer(const char *filename, char delim)
{
  std::vector <std::vector <std::string> > csv_matrix_strings;
  try {
    csv_matrix_strings = process_csv_file(filename, delim);
    if (csv_matrix_strings.size() == 0)
      throw SimulationException("No headings in CSV file.");

    csv_agent_matrix_ = convert_csv_strings_to_reals(csv_matrix_strings);
    // Process header
    bool num_found = false;
    csv_agent_col_headings_ = csv_matrix_strings[0];
    for (size_t i = 0; i < csv_agent_col_headings_.size(); ++i) {
      if (csv_agent_col_headings_[i] == "#") {
	if (num_found == true)
	  throw SimulationException("Two rows with number of agents in csv.");
	csv_num_agents_col_ = i;
	num_found = true;
      } else {
	if (names_states.find(csv_agent_col_headings_[i]) ==
	    names_states.end())
	  throw SimulationException("CSV key not found in state table");
      }
    }
    if (num_found == false)
      throw SimulationException("No row with number of agents in csv.");
  } catch (std::exception &e) {
    throw SimulationException(e.what());
  }
}
