#ifndef SIM_COMMON_H
#define SIM_COMMON_H

#include <functional>
#include <list>
#include <random>
#include <unordered_map>
#include <vector>
#include <tuple>

namespace sim {
  typedef double real;

  enum Parameters {
    INTERIM_REPORT_PARM = 0,
    START_DATE_PARM,
    MORTALITY_RISK_PARM,
    TIME_STEP_SIZE_PARM,
    NUM_TIME_STEPS_PARM,
    PROB_MALE_PARM,
    LAST_PARM
  };
  const unsigned NUM_MORTALITY_PARMS = 120;
  enum States {
    CURRENT_DATE_STATE = 0,
    DOB_STATE,
    ALIVE_STATE,
    DEATH_AGE_STATE,
    SEX_STATE,
    LAST_STATE
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

  class Report;

  // This looks scary but is simply an initializer list of pairs of integers and
  // probability distributions. For example:
  //   perturber dists = {
  //      {SOME_PARM_1, uniform_real_distribution<>(-100.0, 100.0)},
  //      {SOME_PARM_5, weibull_distribution<>(1, 20.0)},
  //      {SOME_PARM_8, normal_distribution<>(40, 20)}
  //   };
  // Where SOME_PARM_1 etc are parameter enumerations.
  // It is used for Monte Carlo simulation.
  #ifdef SIM_VECTORIZE
  typedef std::vector< std::vector< real > > ParameterMap;
  typedef std::vector< std::vector< real > > StateMap;
  #else
  typedef std::unordered_map<unsigned, std::vector< real > > ParameterMap;
  typedef std::unordered_map<unsigned, std::vector< real > > StateMap;
  #endif
  typedef std::initializer_list <
    std::pair < std::vector<double>::size_type,
		std::function<double(std::mt19937_64 &)>> > Perturbers;
  typedef std::function < void(Simulation *) > GlobalEvent;
  typedef std::list< GlobalEvent > GlobalEvents;
  typedef std::function < void(Simulation *) > GlobalStateInit;
  typedef std::function < void(Simulation *, Agent *) >  AgentEvent;
  typedef std::function < void(Agent *, Simulation *) > AgentInit;
  typedef std::list< AgentEvent > AgentEvents;
  typedef std::list< Report > Reports;

  class SimulationException : public std::exception {
  private:
    std::string msg_ = "Simulation exception";
  public:
    SimulationException(const char *s) { msg_ += ": "; msg_ += s; }
    SimulationException() {}
    virtual const char* what() const throw() { return msg_.c_str(); }
  };


  class Report {
  private:
    unsigned frequency_;
    std::function < void(const Simulation *) > report_;
    bool before_;
    bool after_;
  public:
    Report(const std::function < void(const Simulation *) > report,
	   const unsigned frequency = 1,
	   const bool before = true,
	   const bool after = true) : frequency_(frequency), report_(report),
				      before_(before), after_(after) {}
    void operator()(const Simulation *s) { report_(s); }
    unsigned frequency() const { return frequency_; }
    bool before() const { return before_; }
    bool after() const { return after_; }
  };

  class Agent {
  private:
    unsigned long id_;
  public:
    Agent(unsigned long id) : id_(id) {}
    inline unsigned id() const { return id_; }
    StateMap states;
    AgentEvents events;
  };
  extern unsigned thread_num;
  extern thread_local std::mt19937_64 rng;
}

#endif // SIM_COMMON_H
