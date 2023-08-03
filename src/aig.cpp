/*
 * File:  aig.cpp
 * Author:  mikolas
 * Created on:  15 Jan 2017 15:22:48
 * Copyright (C) 2017, Mikolas Janota
 */
#include "aig.h"
#include <algorithm>
#include <utility>      // for swap
#include <iostream>     // for ostream, endl, operator<<, basic_ostream<>::_...
using std::endl;
using std::vector;
AigFactory::AigFactory() {}
AigFactory::~AigFactory() {}

size_t AigFactory::mk_node(AigLit &a, AigLit &b) {
    auto rn = AigNode::mk(a, b);
    const auto i = nodes_map.find(rn);
    if (i != nodes_map.end())
        return i->second;
    const size_t retv = nodes.size();
    nodes.push_back(rn);
    insert_chk(nodes_map, i, rn, retv);
    return retv;
}

void AigFactory::flattened_nodes(AigLit l, vector<AigLit> &res) {
    if (!l.is_node() || l.sign())
        return;
    res.push_back(l);
    const auto n = get_node(l.ptr());
    flattened_nodes(n.a(), res);
    flattened_nodes(n.b(), res);
}

void AigFactory::flatten(AigLit l, vector<AigLit> &res) {
    if (!l.is_node() || l.sign()) {
        res.push_back(l);
        return;
    }
    const auto n = get_node(l.ptr());
    flatten(n.a(), res);
    flatten(n.b(), res);
}

std::ostream &AigFactory::print(std::ostream &out, AigLit l, size_t off) {
    for (size_t i = 0; i < off; ++i)
        out << ' ';
    if (l.is_false())
        return out << "<F>";
    if (l.is_true())
        return out << "<T>";
    if (l.is_var()) {
        return out << (l.sign() ? '-' : '+') << l.var();
    }
    if (l.is_node()) {
        out << (l.sign() ? "-" : "+") << "AND";
        const auto n = get_node(l.ptr());
        vector<AigLit> ops;
        flatten(n.a(), ops);
        flatten(n.b(), ops);
        std::sort(ops.begin(), ops.end());
        for (const auto &op : ops)
            if (op.is_var())
                print(out << " ", op, 0);
        for (const auto &op : ops)
            if (!op.is_var())
                print(out << endl, op, off + 2);
        return out;
    }
    assert(0);
    return out;
}

std::ostream &AigFactory::print(std::ostream &out, AigLit l, size_t off,
                                bool labels, bool pol) {
    for (size_t i = 0; i < off; ++i)
        out << ' ';
    if (l.is_const())
        return out << (pol == l.is_true() ? "<T>" : "<F>");
    const bool npol = pol == !l.sign();
    if (l.is_var()) {
        return out << (npol ? '+' : '-') << l.var();
    }
    if (l.is_node()) {
        const auto n = get_node(l.ptr());
        vector<AigLit> flattened;
        flattened_nodes(n.a(), flattened);
        flattened_nodes(n.b(), flattened);
        vector<AigLit> ops;
        flatten(n.a(), ops);
        flatten(n.b(), ops);
        std::sort(ops.begin(), ops.end());
        out << (npol ? "AND" : "OR");
        if (labels) {
            out << (npol ? "AND" : "OR") << '[' << l.ptr();
            for (const auto &fl : flattened)
                out << ',' << fl.ptr();
            out << ']';
        }
        for (const auto& op : ops)
            if (op.is_var())
                print(out << " ", op, 0, labels, npol);
        for (const auto& op : ops)
            if (!op.is_var())
                print(out << endl, op, off + 2, labels, npol);
        return out;
    }
    assert(0);
    return out;
}
