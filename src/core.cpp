/*
 * File:  Core.cpp
 * Author:  mikolas
 * Created on:  22 Mar 2017 11:24:36
 * Copyright (C) 2017, Mikolas Janota
 */
#include "core.h"

const std::vector<Lit> Core::empty_lits;

Core::Core(const LSet &core, bool flip) {
    c = new Rec();
    c->ref_count = 1;
    for (int i = 0; i < core.size(); i++)
        c->insert(flip ? ~core[i] : core[i]);
}

Core::Core(const SATSPC::vec<SATSPC::lbool> &m, const vector<Var> &vs) {
    c = new Rec();
    c->ref_count = 1;
    for (Var v : vs) {
        const lbool val = eval(v, m);
        c->insert(val == l_True ? mkLit(v) : ~mkLit(v));
    }
}
