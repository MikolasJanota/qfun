#pragma once
#include <string>
class Options {
  public:
    Options()
        : verbose(0), accum(0), cyclic(0), proximity(0), rndmodel(0), sample(0),
          learn(0), interval(0), hybrid(0), initial(0), blocking(0), seed(0),
          win_mv_enum(false), external_sat("") {}
    int get_verbose() const { return verbose; }
    int get_help() const { return help; }
    int get_accum() const { return accum; }
    int get_cyclic() const { return cyclic; }
    int get_proximity() const { return proximity; }
    int get_rndmodel() const { return rndmodel; }
    int get_sample() const { return sample; }
    int get_learn() const { return learn; }
    bool get_interval() const { return interval > 0; }
    size_t get_interval_arg() const { return interval; }
    bool get_hybrid() const { return hybrid > 0; }
    size_t get_hybrid_arg() const { return hybrid; }
    bool get_initial() const { return initial > 0; }
    size_t get_initial_arg() const { return initial; }
    bool get_blocking() const { return blocking > 0; }
    size_t get_blocking_arg() const { return blocking; }
    bool get_seed() const { return seed != 0; }
    int get_seed_arg() const { return seed; }
    bool get_win_mv_enum() const { return win_mv_enum; }
    bool get_external_sat() const { return !external_sat.empty(); }
    const std::string &get_external_sat_arg() const { return external_sat; }

    std::string file_name;
    int verbose;
    int help;
    int accum;
    int cyclic;
    int proximity;
    int rndmodel;
    int sample;
    int learn;
    size_t interval;
    size_t hybrid;
    size_t initial;
    size_t blocking;
    int seed;
    bool win_mv_enum;
    std::string external_sat;
};
