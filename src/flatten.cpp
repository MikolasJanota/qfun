/*
 * File:  flatten.cpp
 * Author:  mikolas
 * Created on:  09 Apr 2017 15:46:33
 * Copyright (C) 2017, Mikolas Janota
 */
#include "flatten.h"
#include "substitute.h"
#include"aig_util.h"
void Flatten::operator() (QAigFla& f) {
    assert(mg.clauses.empty());
    FreshVars fv;
    const SATSPC::vec<lbool> dummy;
    SubstituteAndFreshen sub(factory, dummy, fv);
    f.pref.resize(1);
    f.pref[0].first = qt;
    for (Var v : free) sub.mark(v);
    vector<AigLit> subs;
    subs.push_back(qt == UNIVERSAL ? factory.neg(mg.prop) : mg.prop);
    for (const Game& g : mg.gs) {
        for (const Quantification& q : g.p)
            for (Var v : q.second) sub.mark(v);
        subs.push_back(sub(g.m));
        Var nv;
        size_t dl = 1;
        QuantifierType lqt = qt;
        for (size_t l = 0; l < g.p.size(); ++l,++dl) {
            lqt = neg(lqt);
            if (dl >= f.pref.size()) {
                f.pref.resize(dl+1);
                f.pref[dl].first = g.p[l].first;
            }
            VERIFY(f.pref[dl].first == lqt);
            for (Var v : g.p[l].second)
                if (sub.was_freshened(v, nv)) f.pref[dl].second.push_back(nv);
        }
    }

    AigUtil u(factory);
    f.matrix = qt == UNIVERSAL ? u.or_(subs, true) : u.and_(subs, true);
    Var nv;
    for (Var v : free)
        if (sub.was_freshened(v, nv)) f.pref[0].second.push_back(nv);
}
