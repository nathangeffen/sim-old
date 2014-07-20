#include <algorithm>
#include <iostream>

#include "sim.h"

using namespace sim;

void
Simulation::perturb_parameters(const Perturbers& perturbers)
{
  for (auto & perturber : perturbers) {
    std::vector< real >& vals = parameters[perturber.first].values;
    for (size_t i = 0; i < vals.size(); ++i)
      vals[i] = savedParameters_[perturber.first].get(i) + perturber.second(rng);
  }
}


void
Simulation::montecarlo(const Perturbers& perturbers,
		       std::function<bool(const Simulation *,
					  unsigned)> carryon)
{
  // Save parameters
  for (auto & perturber : perturbers)
    std::copy(parameters[perturber.first].values.begin(),
	      parameters[perturber.first].values.end(),
	      std::back_inserter(savedParameters_[perturber.first].values));
  // Run the simulations
  for (int i = 0; carryon(this, i); ++i) {
    perturb_parameters(perturbers);
    simulate();
  }
  // Restore the parameters
  for (auto & perturber : perturbers)
    std::copy(savedParameters_[perturber.first].values.begin(),
	      savedParameters_[perturber.first].values.end(),
	      parameters[perturber.first].values.begin());
}

void
Simulation::simulate()
{
  // Reports at beginning
  for (auto & report : reports)
    if (report.first > 0)
      report.second(this, agents);
  // Simulate
  unsigned iterations = parameters[NUM_TIME_STEPS_PARM].values[0];
  for (unsigned i = 0; i < iterations; ++i) {
    // Global events
    for (const auto & event : events)
      event(this);
    std::vector<size_t> idx;
    for (size_t j = 0; j < agents.size(); ++j)
      idx.push_back(j);
    std::shuffle(idx.begin(), idx.end(), rng);
    for (auto & j : idx)
      for (auto & event : agents[j].events)
	event(this, agents, agents[j]);
    if (parameters[INTERIM_REPORT_PARM].values[0]) {
      for (const auto & report : reports) {
	if ( report.first && ( (i + 1) % report.first == 0 ))
	  report.second(this, agents);
      }
    }
  }
  // Reports at end
  for (auto & report : reports)
    report.second(this, agents);
}
