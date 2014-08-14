#include <algorithm>
#include <iostream>

#include "sim.hh"

using namespace sim;

Agent*
Simulation::append_agent()
{
  Agent *a = new Agent(agent_count_++);
  agents.push_back(a);
  return a;
}

void
Simulation::set_number_agents(const unsigned num_agents)
{
  for (unsigned i = 0; i < num_agents; ++i)
    append_agent();
}


void
Simulation::kill_agent(Agent *a)
{
  dead_agents.push_back(a);
  a = agents.back();
  agents.pop_back();
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
  // Save parameters
  for (auto & perturber : perturbers)
    std::copy(parameters[perturber.first].begin(),
	      parameters[perturber.first].end(),
	      std::back_inserter(savedParameters_[perturber.first]));
  // Run the simulations
  for (int i = 0; carryon(this, i); ++i) {
    perturb_parameters(perturbers);
    set_global_states();
    set_agent_states();
    simulate(num_steps, interim_reports);
  }
  // Restore the parameters
  for (auto & perturber : perturbers)
    std::copy(savedParameters_[perturber.first].begin(),
	      savedParameters_[perturber.first].end(),
	      parameters[perturber.first].begin());
}

void
Simulation::simulate(unsigned num_steps,
		     bool interim_reports)
{
  // Reports at beginning
  for (auto & report : reports)
    if (report.before())
      report(this);
  // Simulate
  unsigned iterations = num_steps;
  for (; iteration_ < iterations; ++iteration_) {
    // Global events
    for (const auto & event : events)
      event(this);
    std::shuffle(agents.begin(), agents.end(), rng);
    for (auto & agent : agents)
      for (auto & event : agent->events)
	event(this, agent);
    if (parameters[INTERIM_REPORT_PARM][0]) {
      for (auto & report : reports) {
	unsigned freq = report.frequency();
	if ( freq && ( (iteration_ + 1) % freq == 0 ))
	  report(this);
      }
    }
  }
  // Reports at end
  for (auto & report : reports)
    if (report.after())
      report(this);
}

Simulation::~Simulation()
{
  for (auto & agent : agents)
    delete agent;
}
