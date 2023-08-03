/*
 * File:  game_cleanup.cpp
 * Author:  mikolas
 * Created on:  03 May 2017 13:40:01
 * Copyright (C) 2017, Mikolas Janota
 */
#include "game_cleanup.h"
#include "occs.h"
#include "substitute.h"
void GameCleanup::run(Game &g) {
    SATSPC::vec<lbool> subs;
    while (1) {
        Occs ocs(factory);
        ocs.run(g.m);
        bool changed = false;
        subs.clear();
        for (auto &q : g.p) {
            auto &vs = q.second;
            size_t r = 0, w = 0;
            while (r < vs.size()) {
                const Var v = vs[r];
                const bool isn = ocs.occs_neg(v);
                const bool isp = ocs.occs_pos(v);
                // std::cerr<<"P:"<<v<<" "<< isp << std::endl;
                // std::cerr<<"N:"<<v<<" "<< isn << std::endl;
                if (isn && isp) {
                    vs[w++] = vs[r++];
                    continue;
                }
                r++;
                if (isn || isp) {
                    changed = true;
                    const bool set1 = isp == (q.first == EXISTENTIAL);
                    // std::cerr<<"ML:"<<v<<" "<< (set1 ? '1' : '0') <<
                    // std::endl;
                    set(v, set1 ? l_True : l_False, subs);
                }
            }
            vs.resize(w);
        }
        if (!changed)
            break;
        Substitute s(factory, subs);
        g.m = s(g.m);
    };
    size_t last = 0; // maitain the first quantifier except for propositional
    for (size_t i = 1; i < g.p.size(); ++i) {
        auto &vs_i = g.p[i].second;
        if (vs_i.empty())
            continue;
        if (g.p[i].first == g.p[last].first) {
            auto &vs_last = g.p[last].second;
            vs_last.insert(vs_last.end(), vs_i.begin(), vs_i.end());
        } else {
            ++last;
            if (last != i)
                g.p[last] = g.p[i];
        }
    }
    size_t new_sz = last + 1;
    if (new_sz == 1 && g.p[0].second.empty())
        new_sz = 0;
    g.p.resize(new_sz);
}

