/*
 * File:  SeqCounter.hh
 * Author:  mikolas
 * Created on:  Thu Dec 1 15:53:16 GMTST 2011
 * Copyright (C) 2011, Mikolas Janota
 */
#pragma once
#include "sat_interface.h"
#include <vector>
using std::vector;

class SeqCounter {
  public:
    SeqCounter(size_t tval, SATSOLVER &solver,
               const vector<SATSPC::Var> &inputs)
        : tval(tval), solver(solver), inputs(inputs) {
        for (size_t i = 0; i < (inputs.size() - 1) * tval; ++i)
            aux.push_back(newv());
    }

    void encode();
    void encode_all0();

  private:
    const size_t tval;
    SATSOLVER &solver;
    const vector<SATSPC::Var> &inputs;
    vector<SATSPC::Var> aux;

    SATSPC::Var newv() { return solver.newVar(); }

    inline SATSPC::Var s(size_t v_index, size_t j) {
        assert(v_index > 0);
        assert(v_index < inputs.size());
        assert(j >= 1);
        assert(j <= tval);
        return aux[(v_index - 1) * tval + j - 1];
    }

    inline SATSPC::Var v(size_t v_index) {
        assert(v_index > 0);
        assert(v_index <= inputs.size());
        return inputs[v_index - 1];
    }
};
