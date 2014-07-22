#ifndef SIM_H
#define SIM_H

#include <functional>
#include <list>
#include <random>
#include <unordered_map>
#include <vector>

namespace sim {
  typedef double real;

  enum Parameters {
    INTERIM_REPORT_PARM = 0,
    START_DATE_PARM,
    MORTALITY_RISK_PARM,
    TIME_STEP_SIZE_PARM,
    NUM_TIME_STEPS_PARM,
    PROB_MALE_PARM
  };
  const unsigned NUM_MORTALITY_PARMS = 120;
  enum States {
    CURRENT_DATE_STATE = 0,
    DOB_STATE,
    ALIVE_STATE,
    DEATH_AGE_STATE,
    SEX_STATE
  };
  enum gender {
    MALE = 0,
    FEMALE = 1
  };
  enum live_status {
    DEAD = 0,
    ALIVE = 1
  };
  class Agent;
  class Simulation;

  // This looks scary but is simply an initializer list of pairs of integers and
  // probability distributions. For example:
  //   perturber dists = {
  //      {SOME_PARM_1, uniform_real_distribution<>(-100.0, 100.0)},
  //      {SOME_PARM_5, weibull_distribution<>(1, 20.0)},
  //      {SOME_PARM_8, normal_distribution<>(40, 20)}
  //   };
  // Where SOME_PARM_1 etc are parameter enumerations.
  // It is used for Monte Carlo simulation.
  typedef std::initializer_list <
    std::pair < std::vector<double>::size_type,
		std::function<double(std::mt19937_64 &)>> > Perturbers;
  typedef std::function < void(Simulation *) > GlobalEvent;
  typedef std::list< GlobalEvent > GlobalEvents;
  typedef std::function < void(Simulation *, Agent *) >  AgentEvent;
  typedef std::list< AgentEvent > AgentEvents;
  typedef std::pair < unsigned, std::function < void(Simulation *) > > Report;
  typedef std::list< Report > Reports;
  typedef std::unordered_map<unsigned, std::vector< real > > ParameterMap;
  typedef std::unordered_map<unsigned, std::vector< real > > StateMap;

  class Agent {
  public:
    StateMap states;
    AgentEvents events;
  };

  class Simulation {
  protected:
    ParameterMap savedParameters_;
    void perturb_parameters(const Perturbers& peturbers);
  public:
    ~Simulation();
    std::mt19937_64 rng;
    ParameterMap parameters;
    StateMap states;
    GlobalEvents events;
    Reports reports;
    std::vector<Agent *> agents;
    virtual Agent* append_agent();
    virtual void simulate();
    void montecarlo(const Perturbers& peturbers,
		    std::function<bool(const Simulation *,
				       unsigned)> carryon);
  };
}

#endif // SIM_H
