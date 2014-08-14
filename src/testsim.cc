#include <algorithm>
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

enum UserParameters {
  POSITION_INIT_PARM = LAST_PARM + 1,
  POSITION_UPDATE_PARM
};

enum UserStates {
  POSITION_STATE = LAST_STATE + 1
};

tst::TestSeries t("Sim");

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

class UpdatePositionEvent {
public:
  void operator()(Simulation* s, Agent *agent) {
    agent->states[UserStates::POSITION_STATE][0] +=
      s->parameters[POSITION_UPDATE_PARM][0];
    agent->states[UserStates::POSITION_STATE][1] +=
      s->parameters[POSITION_UPDATE_PARM][1];
  }
};


/* REPORTS */

void age_report(const Simulation *s)
{
  int iteration = std::min(s->iteration(),
			   (unsigned) s->parameters.at(NUM_TIME_STEPS_PARM)[0]
			   - 1);
  static real start_date = s->parameters.at(START_DATE_PARM)[0];
  real max_date = 0.0;
  real min_date = 10000000.0;
  real date = s->states.at(CURRENT_DATE_STATE)[0];

  TESTLT(t, fabs(date - (start_date + (iteration + 1) *
			 s->parameters.at(TIME_STEP_SIZE_PARM)[0])),
	 0.000000001,
	 "date incremented");

  for (auto& agent : s->agents) {
    if (agent->states[DOB_STATE][0] < min_date)
      min_date = agent->states[DOB_STATE][0];
    if (agent->states[DOB_STATE][0] > max_date)
      max_date = agent->states[DOB_STATE][0];
  }
  TESTEQ(t, min_date, start_date - 9.0, "minimum date");
  TESTEQ(t, max_date, start_date, "maximum date");
}

void gender_report(const Simulation *s)
{
  size_t num_males = std::count_if(s->agents.begin(), s->agents.end(),
				   [&num_males](const Agent *agent){
				     return agent->states.at(SEX_STATE)[0]
				     == MALE;
				   });
  TESTEQ(t, 0.5, (double) num_males / s->agents.size(),
	 "Number males reasonable.");
}

class PositionReport {
  tst::TestSeries &t_;
public:
  PositionReport(tst::TestSeries &t) : t_(t) {}
  void operator()(const Simulation *s)
  {
    double x, y;
    double i_x = s->parameters.at(POSITION_INIT_PARM)[0];
    double i_y = s->parameters.at(POSITION_INIT_PARM)[1];
    double d_x = s->parameters.at(POSITION_UPDATE_PARM)[0];
    double d_y = s->parameters.at(POSITION_UPDATE_PARM)[1];

    for (auto & a : s->agents) {
      x = i_x + a->id() * d_x + s->iteration() * d_x;
      y = i_y + a->id() * d_y + s->iteration() * d_y;
      TESTLT(t_, a->states.at(POSITION_STATE)[0] - x, 0.0000001,
	     "x position calculated");
      TESTLT(t_, a->states.at(POSITION_STATE)[1] - y, 0.0000001,
	     "y position calculated");
    }
  }
};

/* STATE INITIATION */

void position_state_init(Agent* a, Simulation* s)
{
  // This is an improper deterministic implementation, necessary for testing
  double x = s->parameters[POSITION_INIT_PARM][0];
  double y = s->parameters[POSITION_INIT_PARM][1];
  x += s->parameters[POSITION_UPDATE_PARM][0] * a->id();
  y += s->parameters[POSITION_UPDATE_PARM][1] * a->id();
  a->states[UserStates::POSITION_STATE] = {x, y};
}

void sex_state_init(Agent* a, Simulation* s)
{
  // This is an improper deterministic implementation, necessary for testing
  std::uniform_real_distribution<> dis;
  if (dis(s->rng) < s->parameters[PROB_MALE_PARM][0]) {
    a->states[SEX_STATE] = {MALE};
    s->parameters[PROB_MALE_PARM][0] = 0.0;
  }  else {
    a->states[SEX_STATE] = {FEMALE};
    s->parameters[PROB_MALE_PARM][0] = 1.0;
  }
}

void dob_state_init(Agent *a, Simulation *s)
{
  // This should be done with a Weibull distribution perhaps.
  // This is a deterministic implementation solely for testing purposes.
  static int start = 0;
  a->states[DOB_STATE] = {s->parameters[START_DATE_PARM][0] + start};
  --start;
}

void test_simple_simulation(tst::TestSeries &tst,
			    unsigned num_agents,
			    bool verbose)
{
  Simulation s;
  clock_t t;
  s.initialize({
      // Parameters
      {INTERIM_REPORT_PARM, {1.0}},
	{START_DATE_PARM, {1980.0}},
	  {TIME_STEP_SIZE_PARM, {1.0 / 365}},
	    {NUM_TIME_STEPS_PARM, {20.0 / (1.0 / 365)}},
	      {POSITION_INIT_PARM, {0.0, 0.0}},
		{POSITION_UPDATE_PARM, {1.0, 2.0}},
		  {PROB_MALE_PARM, {1.0}}},
    // Global state initiation functions
    {[](Simulation *s) {
	s->states[CURRENT_DATE_STATE] = {s->parameters[START_DATE_PARM][0]};}},
    {IncrementTimeEvent(1.0 / 365)},
    // Number of agents
    num_agents,
    // Agent state initiation functions
    {sex_state_init, dob_state_init, position_state_init},
    // Agent events
    {UpdatePositionEvent()},
    // Reports
    {{age_report, 1000, false, false},
	{gender_report, 0, true, true},
	  {PositionReport(tst), 0, true, true}});

  TESTEQ(tst, s.states[CURRENT_DATE_STATE][0], 1980.0,
	 "Initial date set");
  TESTLT(tst, s.parameters[NUM_TIME_STEPS_PARM][0] - 7300, 0.001,
			 "time steps set");
  TESTEQ(tst, s.parameters[INTERIM_REPORT_PARM][0], 1.0,
	 "interim reports on");

  t = clock();
  s.simulate(s.parameters[NUM_TIME_STEPS_PARM][0],
	     s.parameters[INTERIM_REPORT_PARM][0]);
  t = clock() - t;
  TESTLT(tst, s.states[CURRENT_DATE_STATE][0] - 2000, 0.00001,
	 "Initial date set");
  TESTLT(tst, s.parameters[NUM_TIME_STEPS_PARM][0] - 7300, 0.00001,
			 "time steps set");
  TESTEQ(tst, s.parameters[INTERIM_REPORT_PARM][0], 1.0,
	 "interim reports on");
  if (verbose) {
    std::clog << "Time for simple simulation: " << (float) t / CLOCKS_PER_SEC
	      << std::endl;
  }
}

void test_monte_carlo(tst::TestSeries &tst,
		      unsigned num_agents,
		      unsigned num_simulations,
		      bool verbose)
{
  Simulation s;
  clock_t t, total_time;
  Perturbers dists = {
    {POSITION_INIT_PARM, std::normal_distribution<>(-2.0, 5.0) },
    {POSITION_UPDATE_PARM, std::normal_distribution<>(-13.5, 10.0) }
  };

  // Set parameters
  s.set_parameters({
      {INTERIM_REPORT_PARM, {1.0}},
      {START_DATE_PARM, {1980.0}},
      {TIME_STEP_SIZE_PARM, {1.0 / 365}},
      {NUM_TIME_STEPS_PARM, {20.0 / (1.0 / 365)}},
      {POSITION_INIT_PARM, {0.0, 0.0}},
      {POSITION_UPDATE_PARM, {1.0, 2.0}},
      {PROB_MALE_PARM, {1.0}}
    });

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
  s.set_agent_initializers({
      sex_state_init, dob_state_init, position_state_init});

  // Set agent events
  s.set_events({UpdatePositionEvent()});

  // Set reports
  s.set_reports({ {PositionReport(tst), 0, true, true} });

  t = clock();
  total_time = clock();
  s.montecarlo(s.parameters[NUM_TIME_STEPS_PARM][0],
	       s.parameters[INTERIM_REPORT_PARM][0],
	       dists,
	       [&num_simulations, &verbose, &t](const Simulation *simulation,
		  unsigned sim_num) {
		 if (sim_num && verbose) {
		   t = clock() - t;
		   std::clog << "Simulations completed: " << sim_num;
		   std::clog << " Time taken: " << (float) t / CLOCKS_PER_SEC
			     << std::endl;
		   t = clock();
		 }
		 return sim_num < num_simulations;
	       });
  total_time = clock() - total_time;
  if (verbose)
    std::clog << "Total Time taken: " << (float) total_time / CLOCKS_PER_SEC
	      << std::endl;
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
  unsigned num_simulations = 1;
  unsigned num_mc_simulations = 8;
  bool verbose = false;
  int opt;

  try {
    while ((opt = getopt(argc, argv, "a:s:m:vh")) != -1) {
      switch (opt) {
      case 'a':
	num_agents = strtou(optarg);
	break;
      case 's':
	num_simulations = strtou(optarg);
	break;
      case 'm':
	num_mc_simulations = strtou(optarg);
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
    for (unsigned i = 0; i < num_simulations; ++i)
      test_simple_simulation(t, num_agents, verbose);

    // Run the Monte Carlo simulation if number of simulations to run > 0
    if (num_mc_simulations > 0)
      test_monte_carlo(t, num_agents, num_mc_simulations, verbose);

    t.summary();
  } catch(std::exception &e) {
    std::cerr << "An Exception occurred: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  if (t.failures())
    return EXIT_FAILURE;
  else
    return EXIT_SUCCESS;
}
