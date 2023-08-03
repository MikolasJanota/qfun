/*
 * File:  Occs.cpp
 * Author:  mikolas
 * Created on:  03 May 2017 11:02:27
 * Copyright (C) 2017, Mikolas Janota
 */
#include "occs.h"
#include <vector>
using std::make_pair;
using std::pair;
using std::vector;

void Occs::run(AigLit l) {
    vector<pair<AigLit, bool>> todo;
    todo.push_back(make_pair(l, false));
    while (!todo.empty()) {
        const pair<AigLit, bool> t = todo.back();
        todo.pop_back();
        const AigLit &l = t.first;
        const bool outer_sign = t.second;
        if (!mark(l, outer_sign))
            continue;
        // factory.print(std::cerr<<"l:"<<endl,l)<<std::endl<<"outer_sign:"<<outer_sign<<std::endl;
        if (!l.is_node())
            continue;
        const auto n = factory.get_node(l.ptr());
        const bool sign = l.sign() != outer_sign;
        // std::cerr<<"my sign:"<<l.sign()<<endl;
        // std::cerr<<"child sign:"<<sign<<endl;
        todo.push_back(make_pair(n.a(), sign));
        todo.push_back(make_pair(n.b(), sign));
    }
}
