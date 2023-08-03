/*
 * File:  LitSet.hh
 * Author:  mikolas
 * Created on:  Wed Oct 12 15:44:23 CEDT 2011
 * Copyright (C) 2011, Mikolas Janota
 */
#pragma once
#include "minisat_auxiliary.h"
#include <iostream>
#include <iterator>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#define DBG_LITSET(t)

class const_LitIterator;

class LitSet {
  public:
    typedef const_LitIterator const_iterator;
    static const LitSet mk(const LiteralVector &ls) { return LitSet(ls); }
    static const LitSet mk(const SATSPC::LSet &ls) { return LitSet(ls); }
    inline LitSet() {
        _literals = nullptr;
        _size = 0;
        _hash_code = EMPTY_HASH;
    }

    inline LitSet(const LitSet &ls) {
        _hash_code = ls._hash_code;
        _size = ls._size;
        _literals = ls._literals;
        if (_literals != nullptr)
            ++(_literals[0].x);
    }

    LitSet &operator=(const LitSet &ls) {
        decrease();
        _hash_code = ls._hash_code;
        _size = ls._size;
        _literals = ls._literals;
        if (_literals != nullptr)
            ++(_literals[0].x);
        return *this;
    }

    static bool is_tautological(const LitSet &a) {
        if (a.size() < 2)
            return false;
        Var v = var(a[0]);
        for (size_t i = 1; i < a.size(); ++i) {
            const auto vi = var(a[i]);
            if (vi == v) {
                assert(a[i] == ~a[i - 1]);
                return true;
            }
            v = vi;
        }
        return false;
    }

    virtual ~LitSet();
    bool equal(const LitSet &other) const;
    std::ostream &print(std::ostream &out) const;
    inline size_t hash_code() const;
    inline size_t size() const;
    inline bool empty() const;
    inline const_iterator begin() const;
    inline const_iterator end() const;
    inline const Lit operator[](size_t index) const;

  private:
    static const size_t EMPTY_HASH = 3889;
    size_t _hash_code;
    size_t _size;
    Lit *_literals;        // first literal as reference counter
    inline int decrease(); // decrease reference counter, deallocat if possible
    LitSet(const LiteralVector &lits);
    LitSet(const SATSPC::LSet &lits);
};

std::ostream &operator<<(std::ostream &outs, const LitSet &ls);

class const_LitIterator : public std::iterator<std::forward_iterator_tag, Lit> {
  public:
    const_LitIterator(const LitSet &ls, size_t x) : ls(ls), i(x) {}
    const_LitIterator(const const_LitIterator &mit) : ls(mit.ls), i(mit.i) {}
    const_LitIterator &operator++() {
        ++i;
        return *this;
    }
    bool operator==(const const_LitIterator &rhs) {
        assert(&ls == &(rhs.ls));
        return i == rhs.i;
    }
    bool operator!=(const const_LitIterator &rhs) {
        assert(&ls == &(rhs.ls));
        return i != rhs.i;
    }
    const Lit operator*() const { return ls[i]; }

  private:
    const LitSet &ls;
    size_t i;
};

class LitSet_equal {
  public:
    inline bool operator()(const LitSet &ls1, const LitSet &ls2) const {
        return ls1.equal(ls2);
    }
};

class LitSet_hash {
  public:
    inline size_t operator()(const LitSet &ls) const { return ls.hash_code(); }
};

typedef std::unordered_map<LitSet, Lit, LitSet_hash, LitSet_equal> LitSet2Lit;
typedef std::unordered_set<LitSet, LitSet_hash, LitSet_equal> LitSetSet;

inline int LitSet::decrease() {
    if (_literals == nullptr)
        return 0;
    assert(_literals[0].x);
    DBG_LITSET(const int ov = _literals[0].x;)
    const int nv = --(_literals[0].x);
    DBG_LITSET(cerr << "dec " << ov << " to " << nv << endl;)
    if ((_literals[0].x) == 0) {
        DBG_LITSET(cerr << "mem rel" << endl;)
        delete[] _literals;
    }
    _literals = nullptr;
    return nv;
}

inline size_t LitSet::hash_code() const { return _hash_code; }
inline size_t LitSet::size() const { return _size; }
inline bool LitSet::empty() const { return _size == 0; }
inline const Lit LitSet::operator[](size_t index) const {
    assert(index < _size);
    return _literals[index + 1];
}

inline LitSet::const_iterator LitSet::begin() const {
    return const_LitIterator(*this, 0);
}

inline LitSet::const_iterator LitSet::end() const {
    return const_LitIterator(*this, _size);
}
