/*
 * File:  aig_util.h
 * Author:  mikolas
 * Created on:  19 Jan 2017 18:12:45
 * Copyright (C) 2017, Mikolas Janota
 */
#pragma once
#include "aig.h"
#include "core.h"
#include "qtypes.h"
class AigUtil {
  public:
    AigUtil(AigFactory &factory) : factory(factory) {}
    AigLit convert(const CNF &cnf);
    AigLit convert_clause(const LitSet &ls);
    inline AigLit convert(const Lit &l) {
        return factory.mk_var(var(l), sign(l));
    }
    AigLit or_lin_(std::vector<AigLit> &xs, bool rmdupl);
    AigLit and_lin_(std::vector<AigLit> &xs, bool rmdupl);
    AigLit or_(std::vector<AigLit> &xs, bool rmdupl);
    AigLit mk_xor(const AigLit &a, const AigLit &b);
    AigLit and_(std::vector<AigLit> &xs, bool rmdupl);
    void rm_dupls(std::vector<AigLit> &xs);
    lbool eval(AigLit fla, const Core &assignment);
    size_t size(const AigLit &fla);
    size_t depth(const AigLit &fla);
    inline AigLit ite(const AigLit &c, const AigLit &t, const AigLit &e) {
        return factory.mk_or(factory.mk_and(c, t),
                             factory.mk_and(factory.neg(c), e));
    }

  private:
    AigFactory &factory;
    std::unordered_map<size_t, size_t> depth_node_cache;
    inline bool get_eval(AigLit t, const Core &assignment,
                         std::unordered_map<size_t, lbool> &node_cache,
                         lbool &rese);
    inline bool aux_sz(const AigLit &t,
                       std::unordered_map<size_t, size_t> &node_cache,
                       size_t &res);
    inline bool aux_depth(const AigLit &t, size_t &res);
};

bool AigUtil::get_eval(AigLit t, const Core &assignment,
                       std::unordered_map<size_t, lbool> &node_cache,
                       lbool &res) {
    if (t.is_node()) {
        const auto ptr = t.ptr();
        const auto i = node_cache.find(ptr);
        if (i == node_cache.end())
            return false;
        res = t.sign() ? lbool_neg(i->second) : i->second;
    } else if (t.is_var()) {
        const Lit vl = get_vlit(t.var(), assignment);
        res = (vl == SATSPC::lit_Undef)
                  ? SATSPC::l_Undef
                  : ((t.sign() == sign(vl)) ? l_True : l_False);
    } else if (t.is_true())
        res = l_True;
    else if (t.is_false())
        res = l_False;
    else
        assert(0);
    return true;
}

inline bool AigUtil::aux_depth(const AigLit &t, size_t &res) {
    if (!t.is_node()) {
        res = 1;
        return true;
    }
    const auto ptr = t.ptr();
    const auto i = depth_node_cache.find(ptr);
    if (i == depth_node_cache.end())
        return false;
    res = i->second;
    return true;
}

inline bool AigUtil::aux_sz(const AigLit &t,
                            std::unordered_map<size_t, size_t> &node_cache,
                            size_t &res) {
    if (!t.is_node()) {
        res = 1;
        return true;
    }
    const auto ptr = t.ptr();
    const auto i = node_cache.find(ptr);
    if (i == node_cache.end())
        return false;
    res = i->second;
    return true;
}
