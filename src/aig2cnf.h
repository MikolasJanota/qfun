/*
 * File:  aig2cnf.h
 * Author:  mikolas
 * Created on:  15 Jan 2017 16:27:59
 * Copyright (C) 2017, Mikolas Janota
 */
#pragma once
#include "aig.h"
#include "minisat/core/SolverTypes.h"
#include "minisat_auxiliary.h"
#include "qtypes.h"
#include "var_manager.h"
#include <iostream> // for basic_ostream::operator<<, ope...
#include <utility>  // for pair
template <class SATSolver> class aig2cnf {
  public:
    aig2cnf(VarManager &vm, Level ql, AigFactory &factory,
            SATSolver &_sat_solver)
        : var_mng(vm), qlevel(ql), factory(factory), sat_solver(_sat_solver),
          max_id(-1), true_lit(SATSPC::lit_Undef) {}

    inline void alloc_vars(Var _max_id) {
        if (max_id > _max_id)
            return;
        max_id = _max_id;
        sat_solver.new_variables(max_id);
    }

    inline Var fresh() {
        const auto retv = sat_solver.newVar();
        assert(max_id < retv);
        max_id = retv;
        return retv;
    }
    inline Lit operator()(const AigLit &l) { return encode(l); }

  private:
    VarManager &var_mng;
    Level qlevel;
    AigFactory &factory;
    SATSolver &sat_solver;
    std::unordered_map<size_t, Lit> representatives;
    Var max_id;
    Lit true_lit;
    Lit encode(const AigLit &l) {
        if (true_lit == SATSPC::lit_Undef) {
            true_lit = SATSPC::mkLit(var_mng.fresh_var(qlevel));
            sat_solver.addClause(true_lit);
        }
        // factory.print(std::cerr <<"encoding:", l, 2)<<std::endl;
        if (factory.is_true(l))
            return true_lit;
        if (factory.is_false(l))
            return ~true_lit;
        if (l.is_var()) {
            if (!(l.var() && l.var() <= max_id))
                std::cerr << "v:" << l.var() << std::endl;
            assert(l.var() && l.var() <= max_id);
            return mkLit(l.var(), l.sign());
        }
        return encode_node_lit(l);
    }

    inline Lit encode_node_lit(const AigLit &l) {
        assert(l.is_node());
        const auto ptr = l.ptr();
        const AigNode node = factory.get_node(ptr);
        const auto i = representatives.find(ptr); // memoize check
        const bool is_new = i == representatives.end();
        const Lit node_enc =
            is_new ? encode_and(node.a(), node.b()) : i->second;
        if (is_new)
            insert_chk(representatives, i, ptr, node_enc);
        return l.sign() ? ~node_enc : node_enc;
    }

    inline Lit encode_and(const AigLit &a, const AigLit &b) {
        const auto ae = encode(a);
        const auto be = encode(b);
        const auto retv = mkLit(var_mng.fresh_var(qlevel)); // representative
        sat_solver.addClause(~ae, ~be, retv);
        sat_solver.addClause(be, ~retv);
        sat_solver.addClause(ae, ~retv);
        return retv;
    }
};
