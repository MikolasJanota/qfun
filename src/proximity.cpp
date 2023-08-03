/*
 * File:  proximity.cpp
 * Author:  mikolas
 * Created on:  10 Mar 2017 13:32:16
 * Copyright (C) 2017, Mikolas Janota
 */
#include "proximity.h"
#include "aig_util.h"
using std::unordered_map;
using std::vector;

inline float div2(float f, size_t k) {
    while (k--)
        f /= 2;
    return f;
}

inline float get_1div2(size_t k) { return div2(1.0, k); }

void merge(vector<Var> &v, const vector<Var> &a, const vector<Var> &b) {
    const auto asz = a.size();
    const auto bsz = b.size();
    size_t i = 0, j = 0;
    while (i < asz && j < bsz) {
        if (a[i] < b[j])
            v.push_back(a[i++]);
        else if (a[i] > b[j])
            v.push_back(b[j++]);
        else {
            v.push_back(a[i]);
            i++;
            j++;
        }
    }
    while (i < asz)
        v.push_back(a[i++]);
    while (j < bsz)
        v.push_back(b[j++]);
}

void Proximity::run(AigLit f) {
    unordered_map<AigLit, vector<Var> *> leafs;
    vector<AigLit> todo;
    todo.push_back(f);
    while (!todo.empty()) {
        const AigLit t = todo.back();
        todo.pop_back();
        const auto i = leafs.find(t);
        if (i != leafs.end())
            continue;
        vector<Var> *tl(nullptr);
        if (t.is_const())
            tl = new vector<Var>();
        else if (t.is_var()) {
            tl = mk_vec(t.var());
        } else {
            assert(t.is_node());
            const auto n = factory.get_node(t.ptr());
            const auto prox = get_1div2(ut.depth(t) - 1);
            const auto ia = leafs.find(n.a());
            const auto ib = leafs.find(n.b());
            if (ia != leafs.end() && ib != leafs.end()) {
                tl = new vector<Var>();
                if (prox > 0.1) {
                    merge(*tl, *(ia->second), *(ib->second));
                    for (size_t i = 0; i < tl->size(); ++i)
                        for (size_t j = i + 1; j < tl->size(); ++j)
                            add(tl->at(i), tl->at(j), prox);
                }
            } else {
                todo.push_back(t);
                todo.push_back(n.a());
                todo.push_back(n.b());
            }
        }
        if (tl)
            insert_chk(leafs, i, t, tl);
    }
    // for (auto p : proximities) std::cerr<<"proximity "<<p.first.first<<" -
    // "<<p.first.second<<" : "<<p.second<<std::endl;
    cleanup_vals(leafs);
}
