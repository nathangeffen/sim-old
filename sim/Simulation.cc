#include <algorithm>
#include <iostream>
#include <cstring>

#include "Simulation.hh"

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
Simulation::set_parameter(unsigned parameter,
			  const std::initializer_list<real> values)
{
  parameters[parameter] = values;
}

void
Simulation::set_parameters(const std::initializer_list<
			   std::pair< unsigned,
			   const std::initializer_list<real> > > parms) {
  for (auto & p : parms)
    set_parameter(p.first, p.second);
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
  events = evnts;
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
  for (auto & agent : agents)
    agent->events = events;
}

void
Simulation::set_reports(const std::initializer_list <report_parms_> reprts)
{
  for (auto & r : reprts)
    reports.push_back(Report(r.report_func, r.iteration, r.before, r.after));
}



void
Simulation::initialize(const std::initializer_list <AgentInit> &init_funcs,
		       const std::initializer_list<AgentEvent> events) {
  set_agent_states(init_funcs);
  set_events(events);
}

void
Simulation::initialize(const unsigned num_agents,
		       const std::initializer_list <AgentInit> &init_funcs,
		       const std::initializer_list<AgentEvent> events) {
  set_number_agents(num_agents);
  initialize(init_funcs, events);
}

void
Simulation::initialize(const std::initializer_list< std::pair<
		       unsigned, const std::initializer_list<real> > > parms,
		       const unsigned num_agents,
		       const std::initializer_list <AgentInit> init_funcs,
		       const std::initializer_list<AgentEvent> events) {
  set_parameters(parms);
  initialize(num_agents, init_funcs, events);
}


void
Simulation::initialize(std::initializer_list< std::pair<
		       unsigned,
		       const std::initializer_list<real> > > parameters,
		       const std::initializer_list<GlobalStateInit>
		       global_state_init_funcs,
		       const std::initializer_list<GlobalEvent> global_events,
		       const unsigned num_agents,
		       const std::initializer_list <AgentInit> agent_init_funcs,
		       const std::initializer_list<AgentEvent> agent_events,
		       const std::initializer_list <report_parms_> reports)
{
  initialize(parameters, num_agents, agent_init_funcs, agent_events);
  set_global_states(global_state_init_funcs);
  set_global_events(global_events);
  set_reports(reports);
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
    initialize_states();
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
  set_agent_states();
}

void
Simulation::simulate(unsigned num_steps,
		     bool interim_reports)
{
  try {
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
    for (const auto & event : events)
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
	for (auto & event : agent->events)
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
Simulation::prob_event(real P1,
		       real T1,
		       real T2) const
{
  return 1 - pow((1 - P1), (T1 / T2));
}

bool
Simulation::is_event(real rand,
		     real P1,
		     real T1,
		     real T2) const
{
  if (rand < prob_event(P1, T1, T2))
    return true;
  else
    return false;
}

bool
Simulation::is_event(real P1,
		     real T1,
		     real T2) const
{
  std::uniform_real_distribution<> uni_dis;
  return is_event(uni_dis(rng), P1, T1, T2);
}


bool
Simulation::is_event(real P1) const
{
  return is_event(P1, 1.0, parameters.at(TIME_STEP_SIZE_PARM)[0]);
}

bool
Simulation::is_event(unsigned parameter) const
{
  return is_event(parameters.at(parameter)[0], 1.0,
		  parameters.at(TIME_STEP_SIZE_PARM)[0]);
}
