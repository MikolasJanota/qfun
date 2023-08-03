/*
 * File:   MiniSatExt.hh
 * Author: mikolas
 *
 * Created on November 29, 2010, 5:40 PM
 */
#pragma once
#include "minisat/core/Solver.h"
#include <vector>
namespace SATSPC {
class MiniSatExt : public Solver {
  public:
    inline void bump(const Var var) { varBumpActivity(var); }
    inline void new_variables(Var max_id);
    inline void new_variables(const std::vector<Var> &variables);
    inline lbool swap_pol(Var v, lbool def);
    inline const LSet &get_conflict() { return conflict; }
    inline const vec<lbool> &get_model() { return model; }
};

inline void MiniSatExt::new_variables(Var max_id) {
    const int target_number = (int)max_id + 1;
    while (nVars() < target_number)
        newVar();
}

inline void MiniSatExt::new_variables(const std::vector<Var> &variables) {
    Var max_id = 0;
    for (const auto v : variables) {
        if (max_id < v)
            max_id = v;
    }
    new_variables(max_id);
}

inline lbool MiniSatExt::swap_pol(Var v, lbool def) {
    assert(v < nVars());
    const auto o = user_pol[v];
    const auto r = o == l_Undef ? def : (o == l_True ? l_False : l_True);
    setPolarity(v, r);
    return r;
}
}; /* namespace SATSPC */

