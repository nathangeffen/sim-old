#include <algorithm>
#include <iostream>
#include <random>

#include "sim.hh"

using namespace sim;

class IncrementTime {
private:
  real time_step_size_;
public:
  IncrementTime(real time_step_size) : time_step_size_(time_step_size) {}
  void operator()(Simulation* s) {
    s->states[CURRENT_DATE_STATE][0] += time_step_size_;
  }
};


void die_event(Simulation *s, Agent *agent)
{
  if (agent->states[ALIVE_STATE][0] == DEAD)
    return;

  std::uniform_real_distribution<> dis;

  real age_d = s->states[CURRENT_DATE_STATE][0] - agent->states[DOB_STATE][0];

  unsigned age = round(age_d);

  unsigned parm_index = NUM_MORTALITY_PARMS * agent->states[SEX_STATE][0] + age;

  real risk = pow((1.0 + s->parameters[MORTALITY_RISK_PARM][parm_index]),
		  s->parameters[TIME_STEP_SIZE_PARM][0]) - 1.0;

  if (dis(s->rng) < risk) {
    agent->states[ALIVE_STATE][0] = DEAD;
    agent->states[DEATH_AGE_STATE] = {age_d};
  }
}


void mortality_parm_init(Simulation *s)
{

  // US life tables.
  // Source: http://www.ssa.gov/oact/STATS/table4c6.html
  // MALES
  s->parameters[MORTALITY_RISK_PARM] = {
      // Males
      0.00699,0.000447,0.000301,0.000233,
	0.000177,0.000161,0.00015,0.000139,0.000123,0.000105,0.000091,0.000096,
	0.000135,0.000217,0.000332,0.000456,0.000579,0.000709,0.000843,0.000977,
	0.001118,0.00125,0.001342,0.001382,0.001382,0.00137,0.001364,0.001362,
	0.001373,0.001393,0.001419,0.001445,0.001478,0.001519,0.001569,0.001631,
	0.001709,0.001807,0.001927,0.00207,0.002234,0.00242,0.002628,0.00286,
	0.003117,0.003396,0.003703,0.004051,0.004444,0.004878,0.005347,0.005838,
	0.006337,0.006837,0.007347,0.007905,0.008508,0.009116,0.009723,0.010354,
	0.011046,0.011835,0.012728,0.013743,0.014885,0.016182,0.017612,0.019138,
	0.020752,0.022497,0.024488,0.026747,0.029212,0.031885,0.034832,0.038217,
	0.042059,0.046261,0.050826,0.055865,0.06162,0.068153,0.075349,0.08323,
	0.091933,0.101625,0.112448,0.124502,0.137837,0.152458,0.168352,0.185486,
	0.203817,0.223298,0.243867,0.264277,0.284168,0.303164,0.320876,0.336919,
	0.353765,0.371454,0.390026,0.409528,0.430004,0.451504,0.474079,0.497783,
	0.522673,0.548806,0.576246,0.605059,0.635312,0.667077,0.700431,0.735453,
	0.772225,0.810837,0.851378,0.893947,

	// Females
	0.005728,0.000373,0.000241,0.000186,0.00015,0.000133,0.000121,0.000112,
	0.000104,0.000098,0.000094,0.000098,0.000114,0.000143,0.000183,0.000229,
	0.000274,0.000314,0.000347,0.000374,0.000402,0.000431,0.000458,0.000482,
	0.000504,0.000527,0.000551,0.000575,0.000602,0.00063,0.000662,0.000699,
	0.000739,0.00078,0.000827,0.000879,0.000943,0.00102,0.001114,0.001224,
	0.001345,0.001477,0.001624,0.001789,0.001968,0.002161,0.002364,0.002578,
	0.0028,0.003032,0.003289,0.003559,0.003819,0.004059,0.004296,0.004556,
	0.004862,0.005222,0.005646,0.006136,0.006696,0.007315,0.007976,0.008676,
	0.009435,0.010298,0.011281,0.01237,0.013572,0.014908,0.01644,0.018162,
	0.020019,0.022003,0.024173,0.026706,0.029603,0.032718,0.036034,
	0.039683,0.043899,0.048807,0.054374,0.060661,0.067751,0.075729,
	0.084673,0.094645,0.105694,0.117853,0.131146,0.145585,0.161175,0.17791,
	0.195774,0.213849,0.231865,0.249525,0.266514,0.282504,0.299455,
	0.317422,0.336467,0.356655,0.378055,0.400738,0.424782,0.450269,
	0.477285,0.505922,0.536278,0.568454,0.602561,0.638715,0.677038,
	0.71766,0.76072,0.806363,0.851378,0.893947,
	// The denominator for the risks which is one year
	1.0
	};

}


void sex_state_init(Simulation* s, Agent* a)
{
  std::uniform_real_distribution<> dis;
  if (dis(s->rng) < s->parameters[PROB_MALE_PARM][0])
    a->states[SEX_STATE] = {MALE};
  else
    a->states[SEX_STATE] = {FEMALE};
}

void dob_state_init(Simulation *s, Agent *a)
{
  std::weibull_distribution<> dis(1, 20.0);
  a->states[DOB_STATE] = {s->parameters[START_DATE_PARM][0] -
			  std::min(100.0, dis(s->rng))};
}

void alive_report(Simulation *s)
{
  unsigned num_alive = 0, num_males_alive = 0, num_females_alive = 0;
  unsigned min_sex=0, max_sex=5000;
  real max_age = 0;
  real min_age = 5000;
  real total_age = 0;

  std::cout << "Alive Report" << std::endl;
  std::cout << "Date:\t" << s->states[CURRENT_DATE_STATE][0] << std::endl;
  for (auto agent : s->agents) {
    if (agent->states[ALIVE_STATE][0] == 1.0) {
      real age = s->states[CURRENT_DATE_STATE][0] -
	agent->states[DOB_STATE][0];
      total_age += age;
      if (age > max_age) {
	max_age = age;
	max_sex = agent->states[SEX_STATE][0];
      }
      if (age < min_age) {
	min_age = age;
	min_sex = agent->states[SEX_STATE][0];
      }
      if (agent->states[SEX_STATE][0] == MALE)
	++num_males_alive;
      else
	++num_females_alive;
      ++num_alive;
    }
  }
  std::cout << "Number alive:\t" << num_alive << std::endl;
  std::cout << "Males:\t" << num_males_alive << std::endl;
  std::cout << "Females:\t" << num_females_alive << std::endl;
  std::cout << "Average age:\t" << total_age / num_alive << std::endl;
  std::cout << "Max age:\t" << max_age << "\t"
       << (max_sex == MALE ? "Male\t" : "Female\t")
       << std::endl;
  std::cout << "Min age:\t" << min_age  << "\t"
       << (min_sex == MALE ? "Male\t" : "Female\t")
       << std::endl;
}

void mortality_report(Simulation *s)
{
  std::vector<real> ages, male_ages, female_ages;
  std::cout << "Mortality Report" << std::endl;
  std::cout << "Date:\t" << s->states[CURRENT_DATE_STATE][0] << std::endl;
  for (auto & agent : s->agents)
    if (agent->states[ALIVE_STATE][0] == 0.0) {
      ages.push_back(agent->states[DEATH_AGE_STATE][0]);
      if (agent->states[SEX_STATE][0] == MALE)
	male_ages.push_back(agent->states[DEATH_AGE_STATE][0]);
      else
	female_ages.push_back(agent->states[DEATH_AGE_STATE][0]);
    }


  if (ages.size()) {
    sort(ages.begin(), ages.end());
    std::cout << "Min age:\t" << ages[0] << std::endl;
    std::cout << "Max age:\t" << ages.back() << std::endl;
    std::cout << "Median age:\t" << ages[ages.size() / 2] << std::endl;
  }
  if (male_ages.size()) {
    std::sort(male_ages.begin(), male_ages.end());
    std::cout << "Male min age:\t" << male_ages[0] << std::endl;
    std::cout << "Male max age:\t" << male_ages.back() << std::endl;
    std::cout << "Male median age:\t" << male_ages[male_ages.size() / 2]
	      << std::endl;
  }
  if (female_ages.size()) {
    sort(female_ages.begin(), female_ages.end());
    std::cout << "Female min age:\t" << female_ages[0] << std::endl;
    std::cout << "Female max age:\t" << female_ages.back() << std::endl;
    std::cout << "Female median age:\t" << female_ages[female_ages.size() / 2]
	 << std::endl;
  }
}

int old_main(int argc, char *argv[])
{
  GlobalEvents events;
  Reports reports;
  const unsigned num_agents = argc == 1 ? 100 : atoi(argv[1]);
  Simulation s;

  // Init parameters
  s.parameters[INTERIM_REPORT_PARM].push_back(1.0);
  s.parameters[START_DATE_PARM].push_back(1980.0);
  s.parameters[TIME_STEP_SIZE_PARM].push_back(1.0 / 365);
  s.parameters[NUM_TIME_STEPS_PARM].
    push_back(20.0 / s.parameters[TIME_STEP_SIZE_PARM][0]);
  s.parameters[PROB_MALE_PARM].push_back(0.49);
  // Risk number of days [0]
  s.parameters[MORTALITY_RISK_PARM].push_back(1.0);
  // Set risk of dying at each age
  mortality_parm_init(&s);

  // Init Global States
  s.states[CURRENT_DATE_STATE].push_back(s.parameters[START_DATE_PARM][0]);

  // Init Global Events
  IncrementTime incrementTime(s.parameters[TIME_STEP_SIZE_PARM][0]);
  s.events.push_back(incrementTime);

  // Init agents
  for (unsigned i = 0; i < num_agents; ++i) {
    Agent *a = s.append_agent();;
    sex_state_init(&s, a);
    dob_state_init(&s, a);
    a->states[ALIVE_STATE].push_back(1.0);
    a->events.push_back(die_event);
  }

  // Init reports
  Report r;
  r.first = 1000;
  r.second = alive_report;
  s.reports.push_back(r);
  r.first = 2000;
  r.second = mortality_report;
  s.reports.push_back(r);

  s.simulate();


  std::cout << "START MONTECARLO" << std::endl;
  Perturbers dists = {
    {PROB_MALE_PARM, std::normal_distribution<>(0.1, 0.05) },
    {MORTALITY_RISK_PARM, std::normal_distribution<>(0.0001, 0.0001) }
  };
  s.montecarlo(dists, [](const Simulation *simulation,
			 unsigned iteration) {
		 std::cout << "End of iteration: " << iteration << std::endl;
		 return iteration < 10;
	       });
  std::cout << "END MONTECARLO" << std::endl;
  return 0;
}

int main(int argc, char *argv[])
{

}
