/*
 * File:  FreshVars.h
 * Author:  mikolas
 * Created on:  08 Apr 2017 11:41:57
 * Copyright (C) 2017, Mikolas Janota
 */
#pragma once
#include "sat_interface.h"
#include <iostream>
#include <vector>
class FreshVars {
  public:
    FreshVars() : fresh(1) {}
    virtual ~FreshVars() {}
    bool unmark(SATSPC::Var v) {
        if (!is_marked(v))
            return false;
        const auto i = ix(v);
        marked[i] = false;
        if (ix(fresh) > i)
            fresh = v;
        return true;
    }

    bool mark(SATSPC::Var v) {
        const auto i = ix(v);
        if (i >= marked.size())
            marked.resize(i + 1, false);
        const bool retv = !(marked[i]);
        marked[i] = true;
        while (is_marked(fresh))
            ++fresh;
        return retv;
    }
    bool is_marked(SATSPC::Var v) const {
        const auto i = ix(v);
        return i < marked.size() && marked[i];
    }
    SATSPC::Var mk_fresh() {
        const auto retv = fresh;
        mark(fresh);
        return retv;
    }
    size_t physical_sz() const { return marked.size(); }

  protected:
    SATSPC::Var fresh;
    std::vector<bool> marked;
    inline size_t ix(SATSPC::Var v) const {
        assert(v >= 0);
        return (size_t)v;
    }
};

std::ostream &operator<<(std::ostream &o, const FreshVars &f);
