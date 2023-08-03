/*
 * File:  aig_util.cpp
 * Author:  mikolas
 * Created on:  19 Jan 2017 18:14:45
 * Copyright (C) 2017, Mikolas Janota
 */
#include "aig_util.h"
#include <algorithm>
#include <cassert>
#include <unordered_map>
using std::unordered_map;
AigLit AigUtil::mk_xor(const AigLit &a, const AigLit &b) {
    return factory.mk_and(factory.mk_or(a, b),
                          factory.mk_or(factory.neg(a), factory.neg(b)));
}

AigLit AigUtil::and_lin_(std::vector<AigLit> &xs, bool rmdupl) {
    if (rmdupl)
        rm_dupls(xs);
    auto retv = factory.mk_true();
    for (AigLit &l : xs)
        retv = factory.mk_and(retv, l);
    return retv;
}
AigLit AigUtil::or_lin_(std::vector<AigLit> &xs, bool rmdupl) {
    if (rmdupl)
        rm_dupls(xs);
    auto retv = factory.mk_false();
    for (AigLit &l : xs)
        retv = factory.mk_or(retv, l);
    return retv;
}

AigLit AigUtil::and_(std::vector<AigLit> &xs, bool rmdupls) {
    if (xs.empty())
        return factory.mk_true();
    if (rmdupls)
        rm_dupls(xs);
    vector<AigLit> a2;
    a2.reserve(xs.size() / 2 + (xs.size() & 1));
    auto p1 = &xs;
    auto p2 = &a2;
    while (p1->size() > 1) {
        assert(p2->empty());
        size_t i = 0;
        const auto sz = p1->size();
        while (i + 1 < sz) {
            p2->push_back(factory.mk_and(p1->at(i), p1->at(i + 1)));
            i += 2;
        }
        if (i < sz)
            p2->push_back(p1->at(i));
        p1->clear();
        std::swap(p1, p2);
    }

    assert(p1->size() == 1);
    return p1->at(0);
}

void AigUtil::rm_dupls(std::vector<AigLit> &xs) {
    if (xs.empty())
        return;
    std::sort(xs.begin(), xs.end());
    size_t j = 1;
    AigLit last = xs[0];
    const size_t sz = xs.size();
    for (size_t i = 1; i < sz; ++i) {
        if (xs[i].equals(last))
            continue;
        xs[j] = xs[i];
        last = xs[i];
        ++j;
    }
    assert(j <= sz);
    xs.resize(j);
}

AigLit AigUtil::or_(std::vector<AigLit> &xs, bool rmdupls) {
    //    std::cerr<<"or:"<<std::endl;
    //    for(AigLit l:xs) factory.print(std::cerr, l)<<std::endl;

    if (xs.empty())
        return factory.mk_false();
    vector<AigLit> a2;
    if (rmdupls)
        rm_dupls(xs);
    auto p1 = &xs;
    auto p2 = &a2;
    a2.reserve(p1->size() / 2 + (p1->size() & 1));
    while (p1->size() > 1) {
        assert(p2->empty());
        size_t i = 0;
        const auto sz = p1->size();
        while (i + 1 < sz) {
            p2->push_back(factory.mk_or(p1->at(i), p1->at(i + 1)));
            i += 2;
        }
        if (i < sz)
            p2->push_back(p1->at(i));
        p1->clear();
        std::swap(p1, p2);
    }

    assert(p1->size() == 1);
    return p1->at(0);
}

AigLit AigUtil::convert(const CNF &cnf) {
    vector<AigLit> a1;
    for (const LitSet &ls : cnf)
        a1.push_back(convert_clause(ls));
    const auto retv = and_(a1, false);
    return retv;
}

AigLit AigUtil::convert_clause(const LitSet &ls) {
    vector<AigLit> a1(ls.size());
    for (size_t i = 0; i < ls.size(); ++i)
        a1[i] = convert(ls[i]);
    return or_(a1, false);
}

lbool AigUtil::eval(AigLit fla, const Core &assignment) {
    vector<AigLit> todo;
    todo.push_back(fla);
    unordered_map<size_t, lbool> node_cache;
    lbool tmp;
    while (!todo.empty()) {
        const AigLit t = todo.back();
        todo.pop_back();
        if (get_eval(t, assignment, node_cache, tmp))
            continue;
        assert(t.is_node());
        const auto ptr = t.ptr();
        const auto n = factory.get_node(ptr);
        lbool ares, bres;
        const auto ac = get_eval(n.a(), assignment, node_cache, ares);
        const auto bc = get_eval(n.b(), assignment, node_cache, bres);
        if (ac && bc) {
            insert_chk(node_cache, ptr, lbool_and(ares, bres));
        } else {
            todo.push_back(t);
            todo.push_back(n.a());
            todo.push_back(n.b());
        }
    }
    if (!get_eval(fla, assignment, node_cache, tmp))
        assert(0);
    return tmp;
}

size_t AigUtil::size(const AigLit &fla) {
    vector<AigLit> todo;
    todo.push_back(fla);
    unordered_map<size_t, size_t> node_cache;
    size_t tmp(-1);
    while (!todo.empty()) {
        const AigLit t = todo.back();
        todo.pop_back();
        if (aux_sz(t, node_cache, tmp))
            continue;
        const auto ptr = t.ptr();
        const auto n = factory.get_node(ptr);
        size_t ares, bres;
        const auto ac = aux_sz(n.a(), node_cache, ares);
        const auto bc = aux_sz(n.b(), node_cache, bres);
        if (ac && bc) {
            insert_chk(node_cache, ptr, ares + bres);
        } else {
            todo.push_back(t);
            todo.push_back(n.a());
            todo.push_back(n.b());
        }
    }
    if (!aux_sz(fla, node_cache, tmp))
        assert(0);
    return tmp;
}

size_t AigUtil::depth(const AigLit &fla) {
    vector<AigLit> todo;
    todo.push_back(fla);
    size_t tmp(-1);
    while (!todo.empty()) {
        const AigLit t = todo.back();
        todo.pop_back();
        if (aux_depth(t, tmp))
            continue;
        const auto ptr = t.ptr();
        const auto n = factory.get_node(ptr);
        size_t ares, bres;
        const auto ac = aux_depth(n.a(), ares);
        const auto bc = aux_depth(n.b(), bres);
        if (ac && bc) {
            insert_chk(depth_node_cache, ptr, std::max(ares, bres) + 1);
        } else {
            todo.push_back(t);
            todo.push_back(n.a());
            todo.push_back(n.b());
        }
    }
    if (!aux_depth(fla, tmp))
        assert(0);
    return tmp;
}
