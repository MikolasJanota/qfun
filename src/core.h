/*
 * File:  Core.h
 * Author:  mikolas
 * Created on:  22 Mar 2017 11:24:31
 * Copyright (C) 2017, Mikolas Janota
 */
#pragma once
#include "minisat_auxiliary.h"
#include <vector>
using SATSPC::LSet;

/*
 * A set of literals implemented with reference counting and containing two
 * redundant representations: a factor of literals and a bit-vector of literals,
 * ensuring thus fast access as well as iterations.
 */
class Core {
  public:
    typedef std::vector<Lit>::const_iterator const_iterator;
    Core(const LSet &c, bool flip);
    Core(const SATSPC::vec<SATSPC::lbool> &m, const vector<Var> &vs);
    inline bool has(Lit l) const {
        if (!c)
            return false;
        const size_t li = literal_index(l);
        return li < c->bv.size() && c->bv[li];
    }
    size_t size() const { return c ? c->lits.size() : 0; }
    Lit operator[](size_t i) const { return c->lits[i]; }
    const_iterator begin() const {
        return c ? c->lits.begin() : empty_lits.begin();
    }
    const_iterator end() const { return c ? c->lits.end() : empty_lits.end(); }
    Core() : c(nullptr) {}
    Core(const Core &o) {
        if ((c = o.c))
            c->inc_ref();
    }
    virtual ~Core() { decrease(); }

    Core &operator=(const Core &o) {
        decrease();
        if ((c = o.c))
            c->inc_ref();
        return *this;
    }

    inline void decrease() {
        if (c && c->dec_ref() == 0) {
            delete c;
            c = nullptr;
        }
    }

  protected:
    static const std::vector<Lit> empty_lits;
    struct Rec {
        std::vector<bool> bv;
        std::vector<Lit> lits;
        size_t ref_count;
        size_t inc_ref() { return ++ref_count; }
        size_t dec_ref() { return --ref_count; }
        inline bool insert(Lit l) {
            const size_t li = literal_index(l);
            if (li >= bv.size())
                bv.resize(li + 1, false);
            if (bv[li])
                return false;
            bv[li] = true;
            lits.push_back(l);
            return true;
        }
    };
    Rec *c;
};

inline Lit get_vlit(SATSPC::Var v, const Core &core) {
    const auto pl = SATSPC::mkLit(v, false);
    const auto nl = SATSPC::mkLit(v, true);
    if (core.has(pl))
        return pl;
    if (core.has(nl))
        return nl;
    return SATSPC::lit_Undef;
}

inline std::ostream &operator<<(std::ostream &o, const Core &core) {
    o << '[';
    for (size_t i = 0; i < core.size(); ++i)
        o << (i ? " " : "") << core[i];
    return o << ']';
}
