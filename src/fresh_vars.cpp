/*
 * File:  FreshVars.cpp
 * Author:  mikolas
 * Created on:  08 Apr 2017 11:42:01
 * Copyright (C) 2017, Mikolas Janota
 */
#include "fresh_vars.h"
#include "auxiliary.h"
#include "minisat_auxiliary.h"

std::ostream &operator<<(std::ostream &o, const FreshVars &f) {
    const auto m = static_cast<SATSPC::Var>(f.physical_sz());
    bool fst = true;
    o << '[';
    for (Var v = 0; v < m; ++v) {
        if (!f.is_marked(v))
            continue;
        if (fst)
            fst = false;
        else
            o << ' ';
        o << v;
    }
    return o << ']';
}
