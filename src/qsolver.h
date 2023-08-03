/*
 * File:  qsolver.h
 * Author:  mikolas
 * Created on:  10 Jan 2017 14:41:55
 * Copyright (C) 2017, Mikolas Janota
 */
#pragma once
#include "aig.h"
#include "encoder.h"
#include "level_info.h"
#include "level_solver.h"
#include "options.h"
#include "proximity_table.h"
#include "qtypes.h"
#include "sat_interface.h"
#include "var_manager.h"
#include "varset.h"
#include <cassert>
#include <vector>

class QSolver : public VarManager {
  public:
    QSolver(const QAigFla &f, const Options &options, AigFactory &factory);
    virtual ~QSolver();
    bool solve();
    size_t get_conflicts() const { return conflicts; }
    inline void alloc_var(Var v, Level ql);
    inline Var fresh_var(Level ql);
    std::ostream &prn_refs(std::ostream &o);

  private:
    QAigFla formula;
    const Options &options;
    const int verb;
    AigFactory &factory;
    LevelInfo level_info;
    ProximityTable prox;
    size_t conflicts;
    std::vector<LevelSolver *> solvers;
    void init();
    LevelSolver *get_solver(Level l) {
        assert(0 <= l && (size_t)l < formula.pref.size());
        const auto ix = (size_t)l;
        return solvers[ix];
    }
    Level backtrack(const SATSPC::LSet &core, vec<Lit> &assumptions,
                    Level level);
    void update_assumptions(vec<Lit> &assumptions, Level level,
                            const LevelSolver &s);
    bool _solve();
};

void QSolver::alloc_var(Var v, Level ql) {
    const auto qt = level_info.level_type(ql);
    for (LevelSolver *s : solvers)
        if (s->get_qt() == qt)
            s->alloc_var(v);
}

Var QSolver::fresh_var(Level ql) {
    const auto qt = level_info.level_type(ql);
    bool f = true;
    Var retv = -1;
    for (LevelSolver *s : solvers) {
        if (s->get_qt() != qt)
            continue;
        const auto tmp = s->fresh_var(ql);
        if (f) {
            retv = tmp;
            f = false;
        } else {
            assert(tmp == retv);
        }
    }
    assert(retv >= 0);
    return retv;
}
