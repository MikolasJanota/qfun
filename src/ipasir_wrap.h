/*
 * File:  ipasir_wrap.h
 * Author:  mikolas
 * Created on:  Wed Jul 10 16:42:31 DST 2019
 * Copyright (C) 2019, Mikolas Janota
 */
#pragma once
#define LOGIPASIR(code)                                                        \
    do {                                                                       \
        /* code */                                                             \
    } while (0)

#include "ipasir.h"
#include "minisat/core/SolverTypes.h"
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#define SATSPC Minisat
namespace SATSPC {
class IPASIRWrap {
  public:
    std::unordered_map<std::string, SATSPC::Lit> *d_representatives = nullptr;
    std::unordered_map<SATSPC::Var, std::string> *d_inverse_representatives =
        nullptr;
    inline const Minisat::LSet &get_conflict() { return _conflict; }
    inline const Minisat::vec<Minisat::lbool> &get_model() { return _model; }

    void set_representatives(
        std::unordered_map<std::string, SATSPC::Lit> *representatives) {
        assert(!d_representatives);
        d_representatives = representatives;
        d_inverse_representatives =
            new std::unordered_map<SATSPC::Var, std::string>();
        d_inverse_representatives->insert({1, "TRUE"});
        for (const auto &i : *representatives) {
            assert(!sign(i.second));
            d_inverse_representatives->insert({var(i.second), i.first});
        }
    }

    IPASIRWrap() : _nvars(0) { _s = ipasir_init(); }

    virtual ~IPASIRWrap() {
        ipasir_release(_s);
        if (d_inverse_representatives) {
            delete d_inverse_representatives;
        }
    }
    bool addClause(Minisat::vec<Minisat::Lit> &cl) {
        for (int i = 0; i < cl.size(); ++i)
            add(cl[i]);
        return f();
    }

    inline void setPolarity(Var, lbool) {}
    inline void bump(Var) {}
    inline void releaseVar(Lit l) { addClause(l); }
    inline bool simplify() { return true; }
    inline void setFrozen(Var, bool) {}

    inline bool addClause_(Minisat::vec<Minisat::Lit> &cl) {
        return addClause(cl);
    }

    inline bool addClause(const std::vector<Minisat::Lit> &cl) {
        for (const auto &l : cl)
            add(l);
        return f();
    }

    bool addClause(Minisat::Lit p) {
        add(p);
        return f();
    }

    bool addClause(Minisat::Lit p, Minisat::Lit q) {
        add(p);
        add(q);
        return f();
    }

    bool addClause(Minisat::Lit p, Minisat::Lit q, Minisat::Lit r) {
        add(p);
        add(q);
        add(r);
        return f();
    }

    bool addClause(Minisat::Lit p, Minisat::Lit q, Minisat::Lit r,
                   Minisat::Lit s) {
        add(p);
        add(q);
        add(r);
        add(s);
        return f();
    }

    inline Minisat::Var fresh() { return ++_nvars; }
    inline bool is_ok_var(int v) { return 1 <= _nvars && v <= _nvars; }
    int nVars() const { return _nvars; }
    inline Minisat::Var newVar() { return ++_nvars; }
    void new_variables(Var v) {
        if (_nvars < v)
            _nvars = v;
    }

    inline Minisat::lbool eval_lit(const Minisat::Lit &l) const {
        const Minisat::lbool lval = _model[var(l)];
        return lval == Minisat::l_Undef
                   ? Minisat::l_Undef
                   : (Minisat::sign(l) == (lval == Minisat::l_False)
                          ? Minisat::l_True
                          : Minisat::l_False);
    }

    bool solve(const Minisat::vec<Minisat::Lit> &assumps);
    bool solve();
    inline std::ostream &print_literal(std::ostream &output,
                                       const Minisat::Lit &p) {
        output << (sign(p) ? '-' : '+');
        const auto print_name =
            d_representatives && d_inverse_representatives->find(var(p)) !=
                                     d_inverse_representatives->end();
        if (print_name)
            output << "{" << d_inverse_representatives->at(var(p)) << "}";
        else
            output << var(p);
        return output;
    }

  private:
    /* const int           _verb = 1; */
    int _nvars;
    Minisat::Lit _true_lit;
    void *_s;
    Minisat::vec<Minisat::Lit> _assumps;
    Minisat::LSet _conflict;
    Minisat::vec<Minisat::lbool> _model;
    inline int lit2val(const Minisat::Lit &p) {
        return Minisat::sign(p) ? -Minisat::var(p) : Minisat::var(p);
    }

    inline void add(const Minisat::Lit &p) {
        LOGIPASIR(print_literal(std::cerr, p) << " ";);
        ipasir_add(_s, lit2val(p));
    }

    inline bool f() {
        LOGIPASIR(std::cerr << "0[ips]\n";);
        ipasir_add(_s, 0);
        return true;
    }
};

inline bool IPASIRWrap::solve(const Minisat::vec<Minisat::Lit> &assumps) {
    for (int i = 0; i < assumps.size(); ++i) {
        LOGIPASIR(print_literal(std::cerr << "Assumption:", assumps[i])
                      << '\n';);
        ipasir_assume(_s, lit2val(assumps[i]));
    }
    const auto rv = solve();
    LOGIPASIR(std::cerr << "rv:" << (rv ? "SAT" : "UNSAT") << '\n';);
    if (!rv) {
        _conflict.clear();
        for (int i = 0; i < assumps.size(); ++i) {
            const auto val = lit2val(assumps[i]);
            if (ipasir_failed(_s, val))
                _conflict.insert(~assumps[i]);
        }
    }
    return rv;
}

inline bool IPASIRWrap::solve() {
    const int r = ipasir_solve(_s);
    assert(r == 10 || r == 20);
    if (r != 10 && r != 20) {
        std::cerr << "Something went wrong with ipasir_solve call, retv: " << r
                  << std::endl;
        exit(1);
    }
    _model.clear();
    if (r == 10) {
        _model.growTo(_nvars + 1, Minisat::l_Undef);
        for (int v = _nvars; v; v--) {
            const int vval = ipasir_val(_s, v);
            _model[v] = (vval == 0)
                            ? Minisat::l_Undef
                            : (vval < 0 ? Minisat::l_False : Minisat::l_True);
        }
    }
    return r == 10;
}
} // namespace SATSPC
