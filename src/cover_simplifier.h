/*
 * File:  cover_simplifier.h
 * Author:  mikolas
 * Created on:  12 Feb 2017 11:36:54
 * Copyright (C) 2017, Mikolas Janota
 */
#pragma once
#include "aig.h"
#include "aig_util.h"
#include "varset.h"
#include <vector>
class CoverSimplier {
  public:
    CoverSimplier(AigFactory &_f, Var _target_variable, const VarSet &_domain,
                  std::vector<SATSPC::LSet *> &_trues,
                  std::vector<SATSPC::LSet *> &_falses)
        : target_variable(_target_variable), domain(_domain), trues(_trues),
          falses(_falses), factory(_f), util(factory) {}
    virtual ~CoverSimplier() {}
    AigLit run();

  private:
    const Var target_variable;
    const VarSet &domain;
    std::vector<SATSPC::LSet *> &trues;
    std::vector<SATSPC::LSet *> &falses;
    AigFactory &factory;
    AigUtil util;
    AigLit simplify(std::vector<SATSPC::LSet *> &impls);
    // void make_samples(std::vector<SATSPC::LSet*>&
    // positive,std::vector<SATSPC::LSet*>& negative);
    AigLit disjoin(vector<SATSPC::LSet *> &impls);
};
