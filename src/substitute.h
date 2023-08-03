/*
 * File:  substitute.h
 * Author:  mikolas
 * Created on:  07 Apr 2017 09:56:59
 * Copyright (C) 2017, Mikolas Janota
 */
#pragma once
#include "aig.h"
#include "fresh_vars.h"
#include "minisat_auxiliary.h"
#include "varset.h"
class SubstituteBase {
  public:
    SubstituteBase(AigFactory &factory) : factory(factory) {}
    virtual ~SubstituteBase() {}
    AigLit operator()(AigLit l) { return subs(l); }

  protected:
    AigFactory &factory;
    std::unordered_map<size_t, AigLit> node_cache;
    AigLit subs(AigLit fla);
    virtual bool subs_node(const AigLit &t, AigLit &res) = 0;
};

class Substitute : public SubstituteBase {
  public:
    Substitute(AigFactory &factory, const SATSPC::vec<lbool> &s)
        : SubstituteBase(factory), s(s) {}
    virtual ~Substitute() {}
    virtual void mark_used(AigLit l, VarSet &vs);

  protected:
    const SATSPC::vec<lbool> &s;
    virtual bool subs_node(const AigLit &t, AigLit &res);
};

class SubstituteAndFreshen : public Substitute {
  public:
    SubstituteAndFreshen(AigFactory &factory, const SATSPC::vec<lbool> &s,
                         FreshVars &fvars)
        : Substitute(factory, s), FRESHEN(-1), fvars(fvars) {}
    virtual ~SubstituteAndFreshen() {}
    inline void mark(Var v) {
        assert(v != FRESHEN);
        assert(eval(v, s) == l_Undef);
        const auto sz = var_replace.size();
        if (v >= sz) {
            var_replace.growTo(v + 1);
            for (Var i = sz; i < v; ++i)
                var_replace[i] = i;
        }
        var_replace[v] = FRESHEN;
    }

    bool was_freshened(SATSPC::Var v, SATSPC::Var &new_var) {
        new_var = mapped(v);
        return new_var != FRESHEN;
    }

  protected:
    const Var FRESHEN;
    SATSPC::vec<Var> var_replace;
    FreshVars &fvars;
    virtual bool subs_node(const AigLit &t, AigLit &res);
    inline Var mapped(Var v) const {
        return v >= var_replace.size() ? v : var_replace[v];
    }
    inline void map_to(Var v, Var to) {
        assert(var_replace.size() > v);
        var_replace[v] = to;
    }
};

class SubstituteAndFreshenMap : public SubstituteBase {
  public:
    SubstituteAndFreshenMap(AigFactory &factory,
                            const std::unordered_map<Var, AigLit> &s,
                            FreshVars &fvars)
        : SubstituteBase(factory), FRESHEN(-1), s(s), fvars(fvars) {}
    virtual ~SubstituteAndFreshenMap() {}
    inline void mark(Var v) {
        assert(v != FRESHEN);
        assert(!contains(s, v));
        const auto sz = var_replace.size();
        if (v >= sz) {
            var_replace.growTo(v + 1);
            for (Var i = sz; i < v; ++i)
                var_replace[i] = i;
        }
        var_replace[v] = FRESHEN;
    }

    bool was_freshened(SATSPC::Var v, SATSPC::Var &new_var) {
        new_var = mapped(v);
        return new_var != FRESHEN;
    }

  protected:
    const Var FRESHEN;
    const std::unordered_map<Var, AigLit> &s;
    SATSPC::vec<Var> var_replace;
    FreshVars &fvars;
    virtual bool subs_node(const AigLit &t, AigLit &res);
    inline Var mapped(Var v) const {
        return v >= var_replace.size() ? v : var_replace[v];
    }
    inline void map_to(Var v, Var to) {
        assert(var_replace.size() > v);
        var_replace[v] = to;
    }
};
