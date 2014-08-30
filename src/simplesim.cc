/*
  Implementation of some aspects of Granich et al.
  DOI:10.1016/S0140-6736(08)61697-9
 */

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <climits>
#include <cstring>
#include <ctime>
#include <exception>
#include <sstream>
#include <string>
#include <unistd.h>

#include "sim/sim.hh"
#include "test.hh"

using namespace sim;

class ArgException : public std::exception {
private:
  std::string msg_ = "Argument exception";
public:
  ArgException(const char *s) { msg_ += ": "; msg_ += s; }
  ArgException() {}
  virtual const char* what() const throw() { return msg_.c_str(); }
};

// Greek parameters from Figure 2 of Granich et al.
enum UserParameters {
  INITIAL_POP_PARM = LAST_PARM + 1,
  INITIAL_HIV_INFECTION_RATE_PARM,
  HIV_INFECTION_RATE_PARM, // Gamma
  HIV_TRANSITION_PARM,
  BACKGROUND_MORTALITY_PARM
};

enum UserStates {
  HIV_STATE = LAST_STATE + 1,
  HIV_INFECTION_DATE_STATE
};

/* Initialize states */

/* STATE INITIATION */

void dob_state_init(Agent *a, Simulation *s)
{
  std::normal_distribution<double> dis (25.0, 10.0);
  double r = std::max(dis(rng), 15.0);
  a->states[DOB_STATE] = {s->parameters[START_DATE_PARM][0] - r};
}

void alive_state_init(Agent *a, Simulation *s)
{
  a->states[ALIVE_STATE] = {1};
  a->states[DEATH_AGE_STATE] = {0.0};
}

void sex_state_init(Agent* a, Simulation* s)
{
  // This is an improper deterministic implementation, necessary for testing
  std::uniform_real_distribution<> dis;
  if (dis(sim::rng) < s->parameters[PROB_MALE_PARM][0])
    a->states[SEX_STATE] = {MALE};
  else
    a->states[SEX_STATE] = {FEMALE};
}


void hiv_state_init(Agent *a, Simulation *s)
{
  std::uniform_real_distribution<> dis;
  if (dis(sim::rng) <
      s->parameters[INITIAL_HIV_INFECTION_RATE_PARM][0])  {
    a->states[HIV_STATE] = {1};
    a->states[HIV_INFECTION_DATE_STATE] =
      {s->parameters[START_DATE_PARM][0]};
  } else {
    a->states[HIV_STATE] = {0};
    a->states[HIV_INFECTION_DATE_STATE] = {0};
  }
}

/* EVENTS */

/* Global Events */

class IncrementTimeEvent {
private:
  real time_step_size_;
public:
  IncrementTimeEvent(real time_step_size) : time_step_size_(time_step_size) {}
  void operator()(Simulation* s) {
    s->states[CURRENT_DATE_STATE][0] += time_step_size_;
  }
};

/* Agent Events */

void hiv_infection_event(Simulation *s, Agent *a)
{
  bool infected;
  if (a->states[HIV_STATE][0] == 0) {
    infected = s->is_event( (unsigned) HIV_INFECTION_RATE_PARM);
    if (infected) {
      a->states[HIV_STATE][0] = 1;
      a->states[HIV_INFECTION_DATE_STATE][0] =
	s->states[CURRENT_DATE_STATE][0];
    }
  }
}

void hiv_transition_event(Simulation *s, Agent *a)
{
  std::uniform_real_distribution<> dis;
  bool transition;
  if (a->states[UserStates::HIV_STATE][0] > 0 &&
      a->states[UserStates::HIV_STATE][0] < 4) {
    transition = s->is_event( (unsigned) HIV_TRANSITION_PARM);
    if (transition) {
      ++a->states[UserStates::HIV_STATE][0];
    }
  }
}

void death_event(Simulation *s, Agent *a)
{
  double background_mortality = s->parameters[BACKGROUND_MORTALITY_PARM][0];
  double stage4_death = s->parameters[BACKGROUND_MORTALITY_PARM][0];
  bool must_die = false;
  std::uniform_real_distribution<> dis;

  // Risk of death for everyone
  must_die = s->is_event(background_mortality);
  if (must_die == false && a->states[HIV_STATE][0] == 4)
    must_die = s->is_event(stage4_death);
  if (must_die) {
    a->states[ALIVE_STATE][0] = 0;
    a->states[DEATH_AGE_STATE][0] = s->states[CURRENT_DATE_STATE][0];
    s->kill_agent();
  }
}

class DeathEvent {
private:
  double max_age(const double start, const std::vector<Agent *> &agents)
  {
    double m = 0;
    for (auto &a: agents) {
      double age = start - a->states[DOB_STATE][0];
      if (age > m)
	m = age;
    }
    return m;
  }
public:
  void operator()(Simulation* s, Agent *agent)
  {
    static double cutoff = max_age(s->states[CURRENT_DATE_STATE][0],
				    s->agents) * 2;
    double age;

    if (agent->states[ALIVE_STATE][0])
      age = s->states[CURRENT_DATE_STATE][0] - agent->states[DOB_STATE][0];
    if (age > cutoff) {
      agent->states[ALIVE_STATE][0] = 0;
      agent->states[DEATH_AGE_STATE][0] = s->states[CURRENT_DATE_STATE][0];
      s->kill_agent();
    }
  }
};


/* REPORTS */

void mortality_report(const Simulation *s)
{
  size_t num_alive = std::count_if(s->agents.begin(), s->agents.end(),
				   [&num_alive](const Agent *agent){
				     return agent->states.at(ALIVE_STATE)[0];
				   });
  assert(num_alive == s->agents.size());
  size_t num_dead = std::count_if(s->dead_agents.begin(), s->dead_agents.end(),
				  [&num_alive](const Agent *agent){
				    if (agent->states.at(ALIVE_STATE)[0])
				      return false;
				    else
				      return true;
				  });
  assert(num_dead == s->dead_agents.size());
  size_t num_alive_hiv =
    std::count_if(s->agents.begin(), s->agents.end(),
		  [&num_alive](const Agent *agent){
		    return agent->states.at(HIV_STATE)[0] > 0;
		  });
  size_t num_dead_hiv =
    std::count_if(s->dead_agents.begin(), s->dead_agents.end(),
		  [&num_alive](const Agent *agent){
		    return agent->states.at(HIV_STATE)[0] > 0;
		  });
  std::cout << "Alive\tHIV+\tDead\tHIV+" << std::endl;
  std::cout << num_alive << "\t" << num_alive_hiv << "\t"
	    << num_dead << "\t" << num_dead_hiv << std::endl;
}



/* SIMULATION */

void simple_simulation(unsigned num_agents,
		       unsigned num_simulations)
{
  Simulation s;

  // Set parameters
  s.set_parameters({
      {START_DATE_PARM, {2010.0}},
	{TIME_STEP_SIZE_PARM, {1.0 / 365}},
	  {NUM_TIME_STEPS_PARM, {20.0 / (1.0 / 365)}},
	    {PROB_MALE_PARM, {0.5}},
	      {BACKGROUND_MORTALITY_PARM, {0.01}},
		{INITIAL_HIV_INFECTION_RATE_PARM, {0.1}},
		  {HIV_INFECTION_RATE_PARM, {0.02}},
		    {HIV_TRANSITION_PARM, {0.3}}});

  // Set global state functions
  s.set_global_states({
      [](Simulation *s) {
	s->states[CURRENT_DATE_STATE] = {s->parameters[START_DATE_PARM][0]};
      }});

  // Set global events
  s.set_global_events({
      IncrementTimeEvent(s.parameters[TIME_STEP_SIZE_PARM][0])});

  // Set number of agents
  s.set_number_agents(num_agents);

  // Set agent initiation functions
  s.set_agent_initializers({dob_state_init, alive_state_init,
	sex_state_init,  hiv_state_init});

  // Set agent events
  s.set_events({hiv_infection_event, hiv_transition_event, death_event});

  // Set reports
  s.set_reports({
      {mortality_report, 0, true, true} });

  s.initialize_states();
  s.simulate(s.parameters[NUM_TIME_STEPS_PARM][0], false);
  // s.montecarlo(s.parameters[NUM_TIME_STEPS_PARM][0],
  // 	       s.parameters[INTERIM_REPORT_PARM][0],
  // 	       dists,
  // 	       [&num_simulations, &verbose, &t](const Simulation *simulation,
  // 		  unsigned sim_num) {
  // 		 if (sim_num && verbose) {
  // 		   t = clock() - t;
  // 		   std::clog << "Simulations completed: " << sim_num;
  // 		   std::clog << " Time taken: " << (float) t / CLOCKS_PER_SEC
  // 			     << std::endl;
  // 		   t = clock();
  // 		 }
  // 		 return sim_num < num_simulations;
  // 	       });
  return;
}

/* Convert string argument to unsigned. */
unsigned strtou(char *str)
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

 void display_help(const char *prog_name, const char *msg)
{
  if (strcmp(msg, "") != 0)
    std::cerr << msg << std::endl;

  std::cerr << "Microsimulation test program\n\n"
	    << "Usage: "
	    << prog_name
	    << " [-a num_agents] [-s num_simulations] [-v]"
	    << " [-h]\n\n"
	    << "\t-a\tsets the number of agents\n"
	    << "\t-s\tsets the number of simple simulations (0 for none)\n"
	    << "\t-m\tsets the number of Monte Carlo simulations "
	    << "(0 for none)\n"
	    << "\t-v\tprints out verbose information including times\n"
	    << "\t-h\tprints out this help text"
	    << std::endl;
}

int main(int argc, char *argv[])
{
  unsigned num_agents = 12;
  bool verbose = false;
  int opt;

  try {
    while ((opt = getopt(argc, argv, "a:vh")) != -1) {
      switch (opt) {
      case 'a':
	num_agents = strtou(optarg);
	break;
      case 'v':
	verbose = true;
	break;
      case 'h':
	display_help(argv[0], "");
	return EXIT_SUCCESS;
      default:
	throw ArgException();
      }
    }
  } catch (std::exception &e) {
    display_help(argv[0], e.what());
    return EXIT_FAILURE;
  }

  try {
    // Run the simple simulation (default once)
    simple_simulation(num_agents, verbose);
  } catch(std::exception &e) {
    std::cerr << "An Exception occurred: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
