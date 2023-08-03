/*
 * File:  Occs.h
 * Author:  mikolas
 * Created on:  03 May 2017 11:02:23
 * Copyright (C) 2017, Mikolas Janota
 */
#pragma once
#include "aig.h"
#include "varset.h"
#include <unordered_set>
#include <utility> // for pair
class Occs {
  public:
    Occs(AigFactory &factory) : factory(factory) {}
    void run(AigLit l);
    bool occs_pos(Var v) const { return pos_vars.get(v); }
    bool occs_neg(Var v) const { return neg_vars.get(v); }

  private:
    AigFactory &factory;
    std::unordered_set<size_t> pos_nodes;
    std::unordered_set<size_t> neg_nodes;
    VarSet pos_vars;
    VarSet neg_vars;
    inline bool mark(const AigLit &l, bool outer_sign) {
        if (l.is_const())
            return false;
        const bool sign = outer_sign != l.sign();
        if (l.is_node()) {
            auto &ns = sign ? neg_nodes : pos_nodes;
            const std::pair<std::unordered_set<size_t>::iterator, bool> r =
                ns.insert(l.ptr());
            return r.second;
        }
        assert(l.is_var());
        return (sign ? neg_vars : pos_vars).add(l.var());
        assert(0);
        return false;
    }
};
