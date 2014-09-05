/**
 * Use this file as a basis for writing a simulation.
 * The coder's responsibility is to search for all instances of $$ in this file
 * and fill in the appropriate code there if necessary.
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

/* EVENTS */

/* Global Events */

// Insert global events here

// $$ USER TO DO

// Insert agent events here

void death_event(Simulation* s, Agent *agent)
{
  if (agent->states[ALIVE_STATE][0]) {
    // $$ Insert code here to determine if agent should die
    if ( false ) {
      agent->states[ALIVE_STATE][0] = 0;
      agent->states[DEATH_AGE_STATE][0] = s->states[CURRENT_DATE_STATE][0];
	s->kill_agent();
    }
  }
}



/* REPORTS */

void mortality_report(const Simulation *s)
{
  // $$ Replace all this code with your own.

size_t num_alive = std::count_if(s->agents.begin(), s->agents.end(),
	       [](const Agent *agent){
				   return agent->states.at(ALIVE_STATE)[0];
				 });

 size_t num_dead = std::count_if(s->dead_agents.begin(),
				 s->dead_agents.end(),
				 [](const Agent *agent){
				   if (agent->states.at(ALIVE_STATE)[0])
				     return false;
				   else
				     return true;
				 });
 std::cout << "Number alive: " << num_alive << std::endl;
 std::cout << "Number dead: " << num_dead << std::endl;
}

/* STATE INITIATION */

/* $$ Insert other state initiation functions here. */

void alive_state_init(Agent *a, Simulation *s)
{
  a->states[ALIVE_STATE] = {1};
  a->states[DEATH_AGE_STATE] = {0.0};
}


void run_simulation(
		    // Number of simulations to run.
		    // Typically set to 1 unless you're doing
		    // Monte Carlo or some other type of uncertainty analysis.
		    unsigned num_simulations,
		    // Number of agents to be created. 0 if using a CSV file.
		    unsigned num_agents,
		    // Agent comma separated filename.
		    // "" or NULL if not being used.
		    const char * agent_csv_filename
		    )
{
  Simulation s;

  Perturbers sensitivities = {
    // $$ If you wish to do sensitivity testing
    //    on one or more parameters, you will need to specify which
    //    parameters, and how they must be perturbed here.
    //    Here are two commented out examples.
    // {POSITION_INIT_PARM, std::normal_distribution<>(-2.0, 5.0) },
    // {POSITION_UPDATE_PARM, std::normal_distribution<>(-13.5, 10.0) }
  };

  // Set the parameters
  s.set_parameters({
      // $$ You might need to modify some of these parameters
      // Set this to 1.0 if you want any of the reports to be run
      // every specified number of iterations of the simulation.
      {INTERIM_REPORT_PARM, {0.0}, "interim report"},
	// Set this to the start date of the simulation.
	// Default is 1 January 1980.
	{START_DATE_PARM, {1980.0}, "start date"},
	// Set this to the time step. 1.0 is one year. 1.0 / 365 is 1 day.
	// 1.0 / 12 is one month.
	// Default is one day.
	  {TIME_STEP_SIZE_PARM, {1.0 / 365}, "time step size"},
	  // Set this to the number of iterations.
	  // Default is 7300, the number of days in 20 years.
	    {NUM_TIME_STEPS_PARM, {20.0 / (1.0 / 365)}, "num time steps"}
	    // $$ Add other parameters here.
    });

  // Global state initiation functions
  s.set_global_states({
      // Sets the current date for the simulation.
      // Default is taken from the STATE_DATE_PARM. You should not modify
      // this. Rather modify the STATE_DATE_PARM parameter above.
      [](Simulation *s) {
	s->states[CURRENT_DATE_STATE] = {s->parameters[START_DATE_PARM][0]};
      }
      // $$ Add other global state initiation functions here.
    });

  // Global events
  s.set_global_events({
      // This event updates the simulation date on each iteration.
      // Unlikely you'd want to remove it.
      IncrementTimeEvent(s.parameters[TIME_STEP_SIZE_PARM][0])
	// $$ Add events to change global states here.
	});


  // EITHER set the number of agents OR use a CSV file to set the number of
  // agents. If you do both, the code needs to be more complex than this.

  assert(num_agents || (agent_csv_filename && strlen(agent_csv_filename) > 0));
  if (num_agents > 0 && (agent_csv_filename == NULL ||
			 strlen(agent_csv_filename) == 0) )
    // Number of agents
    s.set_number_agents(num_agents);
  else
    // Set CSV file initializer
    s.set_agent_csv_initializer(agent_csv_filename);

  // Agent state initiation functions
  s.set_agent_initializers({
      // Initializes the agents to be alive. Unlikely you'd want to change this.
      alive_state_init
	// $$ Add initializers for other agent states here.
	});

  // Agent events
  s.set_events({
      death_event
	// $$ Add events to change agent states here.
	});

  // Reports
  s.set_reports({
      // $$ You might want to remove the mortality_report or modify
      //    how often it runs.
      // Each entry in this initializer list is a tuple with four elements:
      // 1: the report function
      // 2. How frequently to run it.
      //    Setting this to 1 will run the report every iteration.
      //    Setting it to 0 will mean it is not run at all during the simulation.
      //    Setting it to 10, for example, will mean it is run every
      //    10th iteration.
      // 3. Whether the report must run before the simulation starts.
      // 4. Whether the report must run when the simulation is finished.
      // In the example below, the mortality report is only run at the end
      // of the simulation.
      {mortality_report, 0, false, true}
	// $$ Add more reports here.
    });

  // Now run a montecarlo simulation.
  try {
    s.montecarlo(s.parameters[NUM_TIME_STEPS_PARM][0],
		 s.parameters[INTERIM_REPORT_PARM][0],
		 sensitivities,
		 // $$ You might want to change this lamba function
		 //    if you want something more sophisticated than
		 //    straightforward Monte Carlo uncertainty analysis.
		 [&num_simulations](const Simulation *simulation,
				    const unsigned sim_num) {
		   return sim_num < num_simulations;
		 });
  } catch (std::exception &e) {
    std::cerr << "An exception occurred running the simulation." << std::endl;
    throw e;
  }
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

  // $$ If you add options, update the help
  std::cerr << "Microsimulation test program\n\n"
	    << "Usage: "
	    << prog_name
	    << " [-s num_simulations] [-a num_agents] [-f comma_separated_file]"
	    << " [-h]\n\n"
	    << "\t-s\tNumber of simulations to run\n"
	    << "\t-a\tNumber of agents\n"
	    << "\t-f\tComma separated file for agent initialization\n"
	    << "\t-h\tDisplay this help text"  << std::endl;
}

int main(int argc, char *argv[])
{
  // $$ Default number of agent
  unsigned num_agents = 10000;
  // $$ Default number of simulations
  unsigned num_simulations = 1;
  // $$ Default to no agent csv file
  std::string agent_csv_filename = "";

  int opt;

  try {
    while ((opt = getopt(argc, argv, "s:a:m:f:h")) != -1) {
      switch (opt) {
      case 's': // Number of simulations to run
	num_simulations = strtou(optarg);
	break;
      case 'a': // Number of agents
	num_agents = strtou(optarg);
	break;
      case 'f': // comma separated initialization file for agents
	agent_csv_filename = std::string(optarg);
	break;
      case 'h': // Display help
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
      run_simulation(num_simulations,
		     num_agents,
		     agent_csv_filename.c_str());
      return EXIT_SUCCESS;
    } catch(std::exception &e) {
      std::cerr << "Error: " << e.what() << std::endl;
      return EXIT_FAILURE;
  }
}
