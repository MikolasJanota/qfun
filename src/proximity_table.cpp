/*
 * File:  proximity_table.cpp
 * Author:  mikolas
 * Created on:  14 Mar 2017 09:36:43
 * Copyright (C) 2017, Mikolas Janota
 */
#include "proximity_table.h"
#include "auxiliary.h"
#include <utility>                     // for pair
using std::make_pair;
void ProximityTable::operator () (const Proximity& proximity, const LevelInfo& level_info) {
    const auto& p = proximity.get_proximities();
    for (auto i : p) {
        Var v1 = i.first.first;
        Var v2 = i.first.second;
        if (level_info.depends_on(v2, v1)) std::swap(v1, v2);
        if (!level_info.depends_on(v1, v2)) continue;
        const auto val =  make_pair(v2,i.second);
        const auto ti = m_table.find(v1);
        if (ti==m_table.end()) {
            insert_chk(m_table,ti,v1,mk_vec(val));
        } else {
            ti->second->push_back(val);
        }
    }
}

ProximityTable::~ProximityTable() {
     cleanup_vals(m_table);
}
