/*
 * File:  VarSet.hh
 * Author:  mikolas
 * Created on:  Sun Nov 13 10:47:38 EST 2011
 * Copyright (C) 2011, Mikolas Janota
 */
#ifndef VARSET_HH_12377
#define VARSET_HH_12377
#include "auxiliary.h"
#include "minisat_auxiliary.h"
#include <iterator>
#include <vector>
using SATSPC::Var;

class VarSet;

class const_ord_VarIterator
    : public std::iterator<std::forward_iterator_tag, Var> {
  public:
    inline const_ord_VarIterator(const VarSet &ls, size_t x);
    inline const_ord_VarIterator(const const_ord_VarIterator &mit)
        : ls(mit.ls), i(mit.i) {}
    inline const_ord_VarIterator &operator++();
    inline Var operator*() const;
    bool operator==(const const_ord_VarIterator &rhs) {
        assert(&ls == &(rhs.ls));
        return i == rhs.i;
    }
    bool operator!=(const const_ord_VarIterator &rhs) {
        assert(&ls == &(rhs.ls));
        return i != rhs.i;
    }

  private:
    const VarSet &ls;
    size_t i;
};

class VarSet {
  public:
    typedef std::vector<Var>::const_iterator const_iterator;
    VarSet();
    VarSet(const VarSet &) = delete;
    void add_all(const std::vector<Var> &variables);
    void add_all(const VarSet &o);
    inline bool add(Var v);
    inline bool remove(Var v);
    inline bool get(Var v) const;
    inline void clear();
    inline size_t physical_size() const { return s.size(); }
    inline const_iterator begin() const { return vs.begin(); }
    inline const_iterator end() const { return vs.end(); }
    inline const_ord_VarIterator ord_begin() const;
    inline const_ord_VarIterator ord_end() const;
    inline size_t size() const;
    inline bool empty() const;
    std::ostream &print(std::ostream &out) const;

  private:
    std::vector<bool> s;
    std::vector<size_t> poss;
    std::vector<Var> vs;
};

inline bool VarSet::add(Var v) {
    assert(v > 0);
    const size_t ix = (size_t)v;
    if (s.size() <= ix) {
        s.resize(ix + 1, false);
        poss.resize(ix + 1, -1);
    } else {
        if (s[ix])
            return false;
    }
    s[ix] = true;
    poss[ix] = vs.size();
    vs.push_back(v);
    assert(vs[poss[ix]] == v);
    return true;
}

inline bool VarSet::remove(Var v) {
    assert(v > 0);
    const size_t vix = (size_t)v;
    if (s.size() <= vix || !s[vix])
        return false;
    s[vix] = false;
    const auto v_pos = poss[vix];
    assert(vs[v_pos] == v);
    const auto v_back = vs.back();
    const auto v_back_ix = (size_t)v_back;
    vs[v_pos] = v_back;
    poss[v_back_ix] = v_pos;
    vs.pop_back();
    assert(vs[poss[v_back_ix]] == v_back);
    return true;
}

inline bool VarSet::get(Var v) const {
    assert(v >= 0);
    const auto i = (size_t)v;
    return i < s.size() && s[i];
}

inline size_t VarSet::size() const { return vs.size(); }
inline bool VarSet::empty() const { return vs.empty(); }
inline void VarSet::clear() {
    s.clear();
    vs.clear();
    poss.clear();
}

std::ostream &operator<<(std::ostream &outs, const VarSet &ls);

inline SATSPC::Var maxv(const VarSet &vs) {
    SATSPC::Var retv = -1;
    for (auto v : vs) {
        assert(v >= 0);
        if (v > retv)
            retv = v;
    }
    return retv;
}

inline const_ord_VarIterator::const_ord_VarIterator(const VarSet &ls, size_t x)
    : ls(ls), i(x) {
    while ((i < ls.physical_size()) && !ls.get(i))
        ++i;
}
inline Var const_ord_VarIterator::operator*() const {
    assert(ls.get(i));
    return (Var)i;
}

inline const_ord_VarIterator &const_ord_VarIterator::operator++() {
    assert(i < ls.physical_size());
    ++i;
    while ((i < ls.physical_size()) && !ls.get(i))
        ++i;
    return *this;
}

inline const_ord_VarIterator VarSet::ord_end() const {
    return const_ord_VarIterator(*this, physical_size());
}
inline const_ord_VarIterator VarSet::ord_begin() const {
    return const_ord_VarIterator(*this, 0);
}
#endif /* VARSET_HH_12377 */
