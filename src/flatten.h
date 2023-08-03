/*
 * File:  flatten.h
 * Author:  mikolas
 * Created on:  09 Apr 2017 15:46:27
 * Copyright (C) 2017, Mikolas Janota
 */
#pragma once
#include "aig.h"
#include "qtypes.h"
#include "rareqs_types.h"
#include <vector>                      // for vector
class Flatten {
  public:
    Flatten(AigFactory &factory, QuantifierType qt, std::vector<Var> free,
            const MultiGame &mg)
        : factory(factory), mg(mg), qt(qt), free(free) {}
    virtual ~Flatten() {}
    void operator()(QAigFla &f);

  protected:
    AigFactory &factory;
    const MultiGame &mg;
    QuantifierType qt;
    std::vector<Var> free;
};
