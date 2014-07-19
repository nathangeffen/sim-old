#ifndef SIM_H
#define SIM_H

#include <functional>
#include <list>
#include <random>
#include <unordered_map>
#include <vector>

namespace sim {
  typedef double real;

  const unsigned INTERIM_REPORT_PARM = 0;
  const unsigned START_DATE_PARM = 1;
  const unsigned MORTALITY_RISK_PARM = 2;
  const unsigned TIME_STEP_SIZE_PARM = 3;
  const unsigned NUM_TIME_STEPS_PARM = 4;
  const unsigned PROB_MALE_PARM = 5;

  const unsigned NUM_MORTALITY_PARMS = 120;

  const unsigned CURRENT_DATE_STATE = 0;
  const unsigned DOB_STATE = 1;
  const unsigned ALIVE_STATE = 2;
  const unsigned DEATH_AGE_STATE = 3;
  const unsigned SEX_STATE = 4;

  const unsigned MALE = 0;
  const unsigned FEMALE = 1;

  const unsigned DEAD = 0;
  const unsigned ALIVE = 1;

  class Agent;

  typedef  std::list< std::function < void() > > GlobalEvents;
  typedef  std::list< std::function < void(std::vector<Agent> &,
					   Agent &) > > AgentEvents;
  typedef  std::list< std::pair < unsigned,
	  std::function < void(std::vector<Agent> &) > > > Reports;

  std::mt19937_64 rng;

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

  ParameterMap parameterMap;
  StateMap stateMap;

  class Agent {
  public:
	  DataMap states;
	  AgentEvents events;
  };
}

#endif // SIM_H
