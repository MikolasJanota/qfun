/*
 * File:  proximity_table.h
 * Author:  mikolas
 * Created on:  14 Mar 2017 09:36:36
 * Copyright (C) 2017, Mikolas Janota
 */
#pragma once
#include "level_info.h"
#include "proximity.h"
#include <unordered_map>
#include <vector>
using std::unordered_map;
using std::vector;
class ProximityTable {
  public:
    typedef std::pair<Var, float> Val;
    typedef std::unordered_map<Var, std::vector<Val> *> Table;
    ProximityTable() {}
    void operator()(const Proximity &proximity, const LevelInfo &level_info);
    virtual ~ProximityTable();
    const Table &table() const { return m_table; }

  protected:
    Table m_table;
};
