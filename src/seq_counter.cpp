/*
 * File:  seqcounter.cc
 * Author:  mikolas
 * Created on:  Thu Dec 1 16:19:34 GMTST 2011
 * Copyright (C) 2011, Mikolas Janota
 */
#include "seq_counter.h"
using SATSPC::mkLit;
using SATSPC::Var;

void SeqCounter::encode() {
    const size_t n = inputs.size();
    if (tval == 0) {
        encode_all0();
        return;
    }
    if (tval >= n)
        return;
    assert(n > 1);

    solver.addClause(~mkLit(v(1)), mkLit(s(1, 1)));
    for (size_t j = 2; j <= tval; ++j)
        solver.addClause(~mkLit(s(1, j)));

    for (size_t i = 2; i < n; ++i) {
        solver.addClause(~mkLit(v(i)), mkLit(s(i, 1)));
        solver.addClause(~mkLit(s(i - 1, 1)), mkLit(s(i, 1)));

        for (size_t j = 2; j <= tval; ++j) {
            solver.addClause(~mkLit(v(i)), ~mkLit(s(i - 1, j - 1)),
                             mkLit(s(i, j)));
            solver.addClause(~mkLit(s(i - 1, j)), mkLit(s(i, j)));
        }
        solver.addClause(~mkLit(v(i)), ~mkLit(s(i - 1, tval)));
    }
    solver.addClause(~mkLit(v(n)), ~mkLit(s(n - 1, tval)));
}

void SeqCounter::encode_all0() {
    const size_t n = inputs.size();
    for (size_t i = 1; i <= n; ++i)
        solver.addClause(~mkLit(v(i)));
}
