#ifndef SIM_H
#define SIM_H

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
  typedef std::unordered_map<unsigned, std::vector< real > > ParameterMap;
  typedef std::unordered_map<unsigned, std::vector< real > > StateMap;
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

  class Simulation {
  private:
    unsigned long agent_count_ = 0;
    unsigned long iteration_ = 0;
    std::list <GlobalStateInit> init_global_state_funcs_;
    std::list <AgentInit> init_agent_funcs_;
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
    std::vector<Agent *> dead_agents;
    virtual Agent* append_agent();
    void set_number_agents(const unsigned num_agents);
    inline unsigned iteration() const { return iteration_; }
    void kill_agent(Agent *);
    inline void set_parameter(unsigned parameter,
			       const std::initializer_list<real> values) {
      parameters[parameter] = values;
    }
    inline void set_parameters(const std::initializer_list<
			       std::pair<
			       unsigned,
			       const std::initializer_list<real>
			       >
			       > parms) {
      for (auto & p : parms)
	set_parameter(p.first, p.second);
    }
    inline void set_global_state_initializers(const std::initializer_list
					      <GlobalStateInit>  init_funcs)  {
      init_global_state_funcs_ = init_funcs;
    }
    inline void set_global_states() {
      for (auto & f : init_global_state_funcs_)
	f(this);
    }
    inline void set_global_states(const std::initializer_list<GlobalStateInit>
				   init_funcs) {
      set_global_state_initializers(init_funcs);
      set_global_states();
    }
    inline void set_agent_initializers(const std::initializer_list <AgentInit>
				       init_funcs) {
      init_agent_funcs_ = init_funcs;
    }
    void set_global_events(const std::initializer_list<GlobalEvent> evnts) {
      events = evnts;
    }

    void set_agent_states();
    void set_agent_states(const std::initializer_list <AgentInit> init_funcs);
    void set_events(const std::initializer_list<AgentEvent> events);
    inline void initialize(const std::initializer_list <AgentInit> &init_funcs,
			   const std::initializer_list<AgentEvent> events) {
      set_agent_states(init_funcs);
      set_events(events);
    }
    struct report_parms_ {
      std::function < void(const Simulation *) > report_func;
      unsigned iteration;
      bool before;
      bool after;
    };
    inline void set_reports(const std::initializer_list <report_parms_> reprts) {
      for (auto & r : reprts)
	reports.push_back(Report(r.report_func, r.iteration, r.before, r.after));
    }
    inline void initialize(const unsigned num_agents,
			   const std::initializer_list <AgentInit> &init_funcs,
			   const std::initializer_list<AgentEvent> events) {
      set_number_agents(num_agents);
      initialize(init_funcs, events);
    }
    inline void initialize(const std::initializer_list<
			   std::pair<
			   unsigned,
			   const std::initializer_list<real>
			   >
			   > parms,
			   const unsigned num_agents,
			   const std::initializer_list <AgentInit> init_funcs,
			   const std::initializer_list<AgentEvent> events) {
      set_parameters(parms);
      initialize(num_agents, init_funcs, events);
    }
    void initialize(std::initializer_list<
		    std::pair<
		    unsigned,
		    const std::initializer_list<real>
		    >
		    > parameters,
		    const std::initializer_list<GlobalStateInit>
		    global_state_init_funcs,
		    const std::initializer_list<GlobalEvent> global_events,
		    const unsigned num_agents,
		    const std::initializer_list <AgentInit> agent_init_funcs,
		    const std::initializer_list<AgentEvent> agent_events,
		    const std::initializer_list <report_parms_> reports);
    virtual void simulate(const unsigned num_steps,
			  const bool interim_reports);
    void montecarlo(const unsigned num_steps,
		    const bool interim_reports,
		    const Perturbers& peturbers,
		    std::function<bool(const Simulation *, unsigned)> carryon);
  };
}

#endif // SIM_H
