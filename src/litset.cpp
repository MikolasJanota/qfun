/*
 * File:  litset.cc
 * Author:  mikolas
 * Created on:  Wed Oct 12 15:43:36 CEDT 2011
 * Copyright (C) 2011, Mikolas Janota
 */
#include "litset.h"
#include "minisat/mtl/Sort.h"
using SATSPC::LessThan_default;
using SATSPC::LSet;
using SATSPC::sort;
#define CTOR(COLT, IXT)                                                        \
    LitSet::LitSet(const COLT &lits) {                                         \
        const auto vsz = lits.size();                                          \
        if (vsz == 0) {                                                        \
            _literals = nullptr;                                               \
            _size = 0;                                                         \
            _hash_code = EMPTY_HASH;                                           \
            return;                                                            \
        }                                                                      \
        _literals = new Lit[static_cast<size_t>(vsz) + 1];                     \
        _literals[0].x = 1;                                                    \
        for (IXT i = 0; i < vsz; ++i)                                          \
            _literals[static_cast<size_t>(i) + 1] = lits[i];                   \
        sort(_literals + 1, vsz, LessThan_default<Lit>());                     \
        _size = (size_t)vsz;                                                   \
        size_t j = 2;                                                          \
        Lit last = _literals[1];                                               \
        const size_t conv_sz = (size_t)vsz;                                    \
        for (size_t i = 2; i <= conv_sz; ++i) {                                \
            if (_literals[i] == last)                                          \
                continue;                                                      \
            _literals[j] = _literals[i];                                       \
            last = _literals[i];                                               \
            ++j;                                                               \
        }                                                                      \
        _size = j - 1;                                                         \
        _hash_code = 7;                                                        \
        for (size_t i = 1; i <= _size; ++i)                                    \
            _hash_code = _hash_code * 31 + toInt(_literals[i]);                \
    }

CTOR(LiteralVector, size_t)
CTOR(LSet, int)

bool LitSet::equal(const LitSet &other) const {
    if (other._size != _size) {
        return false;
    }
    if (other._literals == _literals)
        return true;
    for (size_t i = 1; i <= _size; ++i)
        if (_literals[i] != other._literals[i])
            return false;
    return true;
}

LitSet::~LitSet() { decrease(); }

ostream &LitSet::print(ostream &out) const {
    bool f = true;
    for (const auto &l : *this) {
        if (!f)
            out << ' ';
        else
            f = false;
        if (SATSPC::sign(l))
            out << '-';
        out << SATSPC::var(l);
    }
    return out;
}

ostream &operator<<(ostream &outs, const LitSet &ls) { return ls.print(outs); }
