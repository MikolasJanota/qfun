/*
 * File:  proximity.h
 * Author:  mikolas
 * Created on:  10 Mar 2017 13:32:12
 * Copyright (C) 2017, Mikolas Janota
 */
#pragma once
#include "aig.h"
#include "aig_util.h"
#include <unordered_map>
#include <utility> // for pair, make_pair, swap
class Proximity {
  public:
    // typedef std::unordered_map<std::pair<SATSPC::Var,SATSPC::Var>, float> MT;
    typedef std::map<std::pair<SATSPC::Var, SATSPC::Var>, float> MT;
    Proximity(AigFactory &f) : factory(f), ut(f) {}
    virtual ~Proximity() {}
    void run(AigLit f);
    const MT &get_proximities() const { return proximities; }

  private:
    AigFactory &factory;
    AigUtil ut;
    MT proximities;

    void add(Var v1, Var v2, float p) {
        if (v1 > v2)
            std::swap(v1, v2);
        const auto k = std::make_pair(v1, v2);
        MT::iterator i = proximities.find(k);
        if (i == proximities.end())
            proximities.insert(i, std::make_pair(k, p));
        else
            i->second += p;
    }
};
