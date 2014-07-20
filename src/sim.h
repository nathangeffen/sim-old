#ifndef SIM_H
#define SIM_H

#include <functional>
#include <list>
#include <random>
#include <unordered_map>
#include <vector>

namespace sim {
  typedef double real;

  enum parameters {
    INTERIM_REPORT_PARM = 0,
    START_DATE_PARM,
    MORTALITY_RISK_PARM,
    TIME_STEP_SIZE_PARM,
    NUM_TIME_STEPS_PARM,
    PROB_MALE_PARM
  };
  const unsigned NUM_MORTALITY_PARMS = 120;

  const unsigned CURRENT_DATE_STATE = 0;
  const unsigned DOB_STATE = 1;
  const unsigned ALIVE_STATE = 2;
  const unsigned DEATH_AGE_STATE = 3;
  const unsigned SEX_STATE = 4;

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
  typedef std::function < void(Simulation *,
			       std::vector<Agent> &,
			       Agent &) >  AgentEvent;
  typedef std::list< AgentEvent > AgentEvents;
  typedef std::pair < unsigned,
		      std::function < void(Simulation *,
					   std::vector<Agent> &) > > Report;
  typedef std::list< Report > Reports;

  class Data {
  public:
    std::vector < real > values;
    inline real get(const size_t n = 0) const { return values[n]; }
    inline void set(const size_t n, const real & val) { values[n] = val; }
    inline void set(const real & val) { set(0, val); }
    inline void set(const std::initializer_list<real> & list) {
      values = list;
    }
  };

  typedef Data Parameter;
  typedef Data State;

  class DataMap {
  public:
    std::unordered_map<unsigned, Data> dataMap;
    inline Data& operator[] ( const unsigned& k ) { return dataMap[k]; };
    inline Data& operator[] ( unsigned&& k )  { return dataMap[k]; };
    inline real & get(const unsigned & k, const size_t n = 0) {
      return dataMap[k].values[n];
    }
    inline void set(const unsigned & k, const size_t n, const real val) {
      dataMap[k].values[n] = val;
    }
    inline void set(const unsigned & k, const real val) {
      set(k, 0, val);
    }
    inline void set(const unsigned & k,
		    const std::initializer_list<real> & list) {
      dataMap[k].values = list;
    }
  };

  typedef DataMap ParameterMap;
  typedef DataMap StateMap;

  class Agent {
  public:
    DataMap states;
    AgentEvents events;
  };

  class Simulation {
  private:
    ParameterMap savedParameters_;
    void perturb_parameters(const Perturbers& peturbers);
  public:
    std::mt19937_64 rng;
    ParameterMap parameters;
    StateMap states;
    GlobalEvents events;
    Reports reports;
    std::vector<Agent> agents;
    virtual void simulate();
    void montecarlo(const Perturbers& peturbers,
		    std::function<bool(const Simulation *,
				       unsigned)> carryon);
  };
}

#endif // SIM_H
