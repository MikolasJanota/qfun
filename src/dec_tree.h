/*
 * File:  dec_tree.h
 * Author:  mikolas
 * Created on:  10 Apr 2017 10:28:31
 * Copyright (C) 2017, Mikolas Janota
 */
#pragma once
#include "aig.h"
#include "minisat_auxiliary.h"
#include "play.h"
#include "varset.h"
#include <cstdint>
#include <vector>
class DecTree {
  public:
    typedef unsigned long count_t;
    typedef Play T;
    DecTree(AigFactory &factory) : verb(0), factory(factory) {}
    virtual ~DecTree() {}
    AigLit operator()(Var tv, const VarSet &domain);
    void add_sample(const T &s) { samples.push_back(s); }

  protected:
    struct Split {
        std::vector<T> pos, neg;
    };
    const int verb;
    AigFactory &factory;
    std::vector<T> samples;
    void split_opp(const std::vector<T> &s, Var tv, Split &out);
    void split_me(Var d, const vector<DecTree::T> &src,
                  std::vector<DecTree::T> &set0, std::vector<DecTree::T> &set1);
    void build(const Split &s, Var tv, VarSet &domain, Var max_domain_var,
               vector<Lit> &stack, vector<SATSPC::LSet *> &trues,
               vector<SATSPC::LSet *> &falses);
    void calc_gains(const VarSet &domain, Var max_domain_var, const Split &s,
                    std::vector<double> &gains);
    void calc_occs(const VarSet &domain, const vector<T> &sset,
                   std::vector<std::pair<count_t, count_t>> &occs);
    Var pick_decision(const VarSet &domain, Var max_domain_var, const Split &s);
};
