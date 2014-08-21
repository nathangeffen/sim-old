#ifndef SIMULATION_H
#define SIMULATION_H

#include "common.hh"

namespace sim {
  class Simulation {
  private:
    unsigned seed_;
    unsigned long agent_count_ = 0;
    unsigned long iteration_ = 0;
    size_t current_agent_index_;
    std::list <GlobalStateInit> init_global_state_funcs_;
    std::list <AgentInit> init_agent_funcs_;
#ifdef SIM_VECTORIZE
    size_t num_parms_;
    size_t num_states_;
#endif
  protected:
    ParameterMap savedParameters_;
    void perturb_parameters(const Perturbers& peturbers);
  public:
    ParameterMap parameters;
    StateMap states;
    GlobalEvents events;
    Reports reports;
    std::vector<Agent *> agents;
    std::vector<Agent *> dead_agents;
    #ifdef SIM_VECTORIZE
    Simulation(unsigned seed = 13,
	       size_t num_parms = 100,
	       size_t num_states = 100);
    #else
    Simulation(unsigned seed = 13);
    #endif
    ~Simulation();
    virtual Agent* append_agent();
    void set_number_agents(const unsigned num_agents);
    unsigned iteration() const;
    void kill_agent(size_t agent_index_);
    void kill_agent();
    void set_parameter(unsigned parameter,
		       const std::initializer_list<real> values);
    void set_parameters(const std::initializer_list< std::pair<
			unsigned, const std::initializer_list<real> > > parms);
    void set_global_state_initializers(const std::initializer_list
				       <GlobalStateInit>  init_funcs);
    void set_global_states();
    void set_global_states(const std::initializer_list<GlobalStateInit>
			   init_funcs);
    void set_agent_initializers(const std::initializer_list <AgentInit>
				init_funcs);
    void set_global_events(const std::initializer_list<GlobalEvent> evnts);
    void set_agent_states();
    void set_agent_states(const std::initializer_list <AgentInit> init_funcs);
    void set_events(const std::initializer_list<AgentEvent> events);
    struct report_parms_ {
      std::function < void(const Simulation *) > report_func;
      unsigned iteration;
      bool before;
      bool after;
    };

    void set_reports(const std::initializer_list <report_parms_> reprts);
    void initialize(const std::initializer_list <AgentInit> &init_funcs,
			   const std::initializer_list<AgentEvent> events);

    void initialize(const unsigned num_agents,
			   const std::initializer_list <AgentInit> &init_funcs,
			   const std::initializer_list<AgentEvent> events);
    void initialize(const std::initializer_list< std::pair<
		    unsigned, const std::initializer_list<real> > > parms,
		    const unsigned num_agents,
		    const std::initializer_list <AgentInit> init_funcs,
		    const std::initializer_list<AgentEvent> events);
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
#endif
