/*
 * File:  encoder.h
 * Author:  mikolas
 * Created on:  Mon, Sep 14, 2015 6:17:43 PM
 * Copyright (C) 2015, Mikolas Janota
 */
#pragma once
#include "aig2cnf.h"
#include "litset.h"
#include "qtypes.h"
#include "var_manager.h"

template <class SATSolverType> class Encoder {
  public:
    Encoder(VarManager &vm, Level ql, AigFactory &factory,
            SATSolverType &sat_solver)
        : var_mng(vm), aig_encoder(vm, ql, factory, sat_solver),
          sat_solver(sat_solver) {}
    inline void alloc_var(Var _max_id) { aig_encoder.alloc_vars(_max_id); }
    inline Var fresh() { return aig_encoder.fresh(); }

    Lit operator()(const AigLit l) { return encode(l); }

    Lit encode(const AigLit l) { return aig_encoder(l); }

    inline bool add(Lit l) { return sat_solver.addClause(l); }
    inline bool add_(vec<Lit> &cl) { return sat_solver.addClause_(cl); }
    inline bool add(const LitSet &clause) {
        vec<Lit> ls(clause.size());
        int i = 0;
        for (Lit l : clause)
            ls[i++] = l;
        return sat_solver.addClause_(ls);
    }

  private:
    VarManager &var_mng;
    aig2cnf<SATSolverType> aig_encoder;
    SATSolverType &sat_solver;
    LitSet2Lit negations;
};
