/*
 * File:  SatWrapper.h
 * Author:  mikolas
 * Created on:  Wed Apr 24 11:04:48 DST 2019
 * Copyright (C) 2019, Mikolas Janota
 */
#ifndef SATWRAPPER_H_21815
#define SATWRAPPER_H_21815
#include "minisat/core/Solver.h"
#include <iostream>
#include <random>
namespace Minisat {
class SatWrapper {
  public:
    SatWrapper() : nvars(0) {}
    virtual ~SatWrapper() {
        for (auto p : cls)
            delete p;
    }
    inline void new_variables(Var max_id);
    inline void new_variables(const std::vector<Var> &variables);
    bool addClause(const vec<Lit> &ps) { return addClause_(ps); }
    bool addClause_(const vec<Lit> &ps);
    bool addClause(Lit l) {
        std::cerr << "unit: " << (Minisat::sign(l) ? "-" : "")
                  << Minisat::var(l) << " " << std::endl;
        tmp.clear();
        tmp.push(l);
        return addClause_(tmp);
    }
    bool addClause(Lit p, Lit q) {
        tmp.clear();
        tmp.push(p);
        tmp.push(q);
        return addClause_(tmp);
    }
    bool addClause(Lit p, Lit q, Lit r) {
        tmp.clear();
        tmp.push(p);
        tmp.push(q);
        tmp.push(r);
        return addClause_(tmp);
    }
    Var newVar() { return nvars++; }
    Var nVars() const { return nvars; }
    bool solve();
    int random_seed;
    int rnd_init_act;
    vec<lbool> model;
    std::string sat_solver_path;

  protected:
    int nvars;
    vec<Lit> tmp;
    static std::mt19937 rgen;
    std::vector<vec<Lit> *> cls;
};
inline void SatWrapper::new_variables(Var max_id) {
    const int target_number = (int)max_id + 1;
    while (nVars() < target_number)
        newVar();
}

inline void SatWrapper::new_variables(const std::vector<Var> &variables) {
    Var max_id = 0;
    for (const Var v : variables) {
        if (max_id < v)
            max_id = v;
    }
    new_variables(max_id);
}

} // namespace Minisat
#endif /* SATWRAPPER_H_21815 */
