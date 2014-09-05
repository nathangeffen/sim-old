#ifndef SIMULATION_H
#define SIMULATION_H

//#include "common.hh"

namespace sim {
  class Simulation {
  private:
    unsigned seed_;
    unsigned long agent_count_ = 0;
    unsigned long iteration_ = 0;
    size_t current_agent_index_;
    std::list <GlobalStateInit> init_global_state_funcs_;
    std::list <AgentInit> init_agent_funcs_;
    std::vector <std::vector <std::string> > csv_matrix_;
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
    GlobalEvents global_events;
    AgentEvents agent_events;
    Reports reports;
    std::vector<Agent *> agents;
    std::vector<Agent *> dead_agents;
    std::unordered_map<unsigned, std::string> parms_names;
    std::unordered_map<std::string, unsigned> names_parms;
    std::unordered_map<unsigned, std::string> states_names;
    std::unordered_map<std::string, unsigned> names_states;
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
    void set_agents_from_csv();
    unsigned iteration() const;
    void kill_agent(size_t agent_index_);
    void kill_agent();
    void set_parameter(const unsigned parameter,
		       const std::initializer_list<real> values,
		       const char* name = "");
    struct parameter_parms_ {
      const unsigned parameter;
      const std::initializer_list<real> values;
      const char * name;
    };
    void set_parameters(const std::initializer_list< const parameter_parms_>);
    void set_state_name(const unsigned state, const char *name);
    void set_state_names(std::initializer_list< std::pair <
			 const unsigned, const char * > >
			 state_names);
    void set_global_state_initializers(const std::initializer_list
				       <GlobalStateInit>  init_funcs);
    void set_global_states();
    void set_global_states(const std::initializer_list<GlobalStateInit>
			   init_funcs);
    void set_agent_csv_initializer(const char *filename, char delim=',');
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
    void initialize_states();
    virtual void simulate(const unsigned num_steps,
			  const bool interim_reports);
    void montecarlo(const unsigned num_steps,
		    const bool interim_reports,
		    const Perturbers& peturbers,
		    std::function<bool(const Simulation *, unsigned)> carryon);
    // Helper functions
    real prob_event(real P1, real T1, real T2) const;
    real prob_event(unsigned parameter) const;
    bool is_event(real rand, real P1, real T1, real T2) const;
    bool is_event(real P1, real T1, real T2) const;
    bool is_event(unsigned parameter) const;
  };


  /* Commonly used events */

  class IncrementTimeEvent {
  private:
    real time_step_size_;
  public:
    IncrementTimeEvent(real time_step_size) : time_step_size_(time_step_size) {}
    void operator()(Simulation* s) {
      s->states[CURRENT_DATE_STATE][0] += time_step_size_;
    }
  };
}

#endif
