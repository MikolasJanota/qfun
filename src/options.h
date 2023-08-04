#pragma once
#include <string>
class Options {
  public:
    int get_verbose() const { return verbose; }

    bool get_accum() const { return accum; }
    bool get_cyclic() const { return cyclic; }
    bool get_proximity() const { return proximity; }
    bool get_rndmodel() const { return rndmodel; }
    bool get_sample() const { return sample; }
    bool get_learn() const { return learn; }
    bool get_win_mv_enum() const { return win_mv_enum; }

    bool get_interval() const { return interval > 0; }
    size_t get_interval_arg() const { return interval; }

    bool get_initial() const { return initial > 0; }
    size_t get_initial_arg() const { return initial; }

    bool get_blocking() const { return blocking > 0; }
    size_t get_blocking_arg() const { return blocking; }

    bool get_seed() const { return seed != 0; }
    int get_seed_arg() const { return seed; }

    std::string file_name;
    int verbose;
    bool accum;
    bool cyclic;
    bool proximity;
    bool rndmodel;
    bool sample;
    bool learn;
    bool win_mv_enum;

    size_t interval;
    size_t hybrid;
    size_t initial;
    size_t blocking;
    int seed;

    std::string external_sat;
};
