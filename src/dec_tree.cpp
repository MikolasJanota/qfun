/*
 * File:  dec_tree.cpp
 * Author:  mikolas
 * Created on:  10 Apr 2017 10:28:38
 * Copyright (C) 2017, Mikolas Janota
 */
#include "dec_tree.h"
#include "cover_simplifier.h"
#include "minisat_auxiliary.h"
#include <cmath>
#include <limits>
#include <utility> // for pair
double entr(DecTree::count_t a, DecTree::count_t b);
using SATSPC::lit_Undef;
using SATSPC::LSet;
using SATSPC::Var;
using std::endl;
using std::numeric_limits;
using std::pair;
using std::vector;

AigLit DecTree::operator()(Var tv, const VarSet &_domain) {
    LOG(1, OUT << "learn:" << tv << std::endl;);
    vector<Lit> stack;
    Split s;
    split_opp(samples, tv, s);
    vector<LSet *> trues, falses;
    VarSet domain;
    domain.add_all(_domain);
    const Var max_domain_var = maxv(domain);
    build(s, tv, domain, max_domain_var, stack, trues, falses);
    CoverSimplier cs(factory, tv, domain, trues, falses);
    const AigLit retv = cs.run();
    cleanup(trues);
    cleanup(falses);
    if (0)
        std::cout << "end learn:" << tv << std::endl;
    return retv;
}

void add_impl(const vector<Lit> &vs, vector<LSet *> &impls) {
    LSet *i = new LSet();
    for (Lit l : vs)
        i->insert(l);
    impls.push_back(i);
    if (0)
        std::cout << "impl:" << (*i) << std::endl;
}

void DecTree::build(const Split &s, Var tv, VarSet &domain, Var max_domain_var,
                    vector<Lit> &stack, vector<LSet *> &trues,
                    vector<LSet *> &falses) {
    LOG(1, OUT << "pos:" << endl; for (auto l
                                       : s.pos) OUT
                                  << l << endl;
        OUT << "neg:" << endl; for (auto l
                                    : s.neg) OUT
                               << l << endl;);
    if (s.pos.empty()) {
        add_impl(stack, falses);
        return;
    }
    if (s.neg.empty()) {
        add_impl(stack, trues);
        return;
    }
    if (domain.empty()) {
        add_impl(stack, trues.size() > falses.size() ? trues : falses);
        std::cerr << "c WARNING: learning has hit an unclassifiable set"
                  << std::endl;
        return;
    }
    const Var d = pick_decision(domain, max_domain_var, s);
    LOG(1, OUT << "split:" << d << std::endl;);
    Split sd0, sd1;
    split_me(d, s.neg, sd0.neg, sd1.neg);
    split_me(d, s.pos, sd0.pos, sd1.pos);
    VERIFY(domain.remove(d));
    const Lit dlit = mkLit(d);
    stack.push_back(~dlit);
    const Var new_max_domain_var =
        d == max_domain_var ? maxv(domain) : max_domain_var;
    build(sd0, tv, domain, new_max_domain_var, stack, trues, falses);
    pop_chk(stack, ~dlit);
    stack.push_back(dlit);
    build(sd1, tv, domain, new_max_domain_var, stack, trues, falses);
    pop_chk(stack, dlit);
    VERIFY(domain.add(d));
}

void DecTree::split_opp(const std::vector<T> &s, Var tv, Split &out) {
    for (T e : s) {
        const Lit tvl = get_vlit(tv, e.opp);
        if (tvl == lit_Undef)
            continue;
        (sign(tvl) ? out.neg : out.pos).push_back(e);
    }
}

void DecTree::split_me(Var d, const vector<DecTree::T> &src,
                       vector<DecTree::T> &set0, vector<DecTree::T> &set1) {
    for (T s : src) {
        const Lit dl = get_vlit(d, s.me);
        (dl != mkLit(d) ? set0 : set1).push_back(s);
    }
}

void DecTree::calc_gains(const VarSet &domain, Var max_domain_var,
                         const Split &s, vector<double> &gains) {
    const pair<count_t, count_t> zp{0, 0};
    vector<pair<count_t, count_t>> noccs(max_domain_var + 1, zp);
    vector<pair<count_t, count_t>> poccs(max_domain_var + 1, zp);
    calc_occs(domain, s.neg, noccs);
    calc_occs(domain, s.pos, poccs);
    for (Var v : domain) {
        const auto &nc = noccs[v];
        const auto &pc = poccs[v];
        LOG(2, OUT << "occs:" << v << " " << nc.first << ":" << nc.second << " "
                   << pc.first << ":" << pc.second << endl;);
        if ((nc.first == 0 && pc.first == 0) ||
            (nc.second == 0 && pc.second == 0)) {
            gains[v] = numeric_limits<double>::lowest();
            continue;
        }
        const double fn = nc.first;
        const double tn = nc.second;
        const double fp = pc.first;
        const double tp = pc.second;
        const double ftot = fn + fp;
        const double ttot = tn + tp;
        const double tot = ftot + ttot;
        const double vgain =
            -(ftot / tot) * entr(fp, fn) - (ttot / tot) * entr(tp, tn);
        gains[v] = vgain;
        LOG(2, OUT << "gain:" << v << " " << fp << ":" << fn << " " << tp << ":"
                   << tn << "=" << vgain << endl;);
    }
}

void DecTree::calc_occs(
    const VarSet &domain, const vector<DecTree::T> &sset,
    vector<pair<DecTree::count_t, DecTree::count_t>> &occs) {
    for (const T &p : sset) {
        for (Var v : domain) {
            const Lit l = get_vlit(v, p.me);
            if (l == lit_Undef) {
                assert(0);
                continue;
            }
            if (sign(l))
                ++(occs[v].first);
            else
                ++(occs[v].second);
        }
    }
}

Var DecTree::pick_decision(const VarSet &domain, Var max_domain_var,
                           const Split &s) {
    assert(!domain.empty());
    vector<double> gains(max_domain_var + 1, numeric_limits<double>::lowest());
    calc_gains(domain, max_domain_var, s, gains);
    // Pick the variable with the largest gain.
    double bval = numeric_limits<double>::lowest();
    Var bvar = -1;
    for (Var v : domain) {
        const auto val = gains[v];
        if (val > bval) {
            bvar = v;
            bval = val;
        }
    }
    return bvar;
}

double entr(DecTree::count_t a, DecTree::count_t b) {
    if (!a || !b)
        return 0;
    const double t = a + b;
    const double ap = (double)a / t;
    const double bp = (double)b / t;
    return -ap * log(ap) - bp * log(bp);
}
