/*
 * File:  substitute.cpp
 * Author:  mikolas
 * Created on:  07 Apr 2017 09:57:04
 * Copyright (C) 2017, Mikolas Janota
 */
#include"substitute.h"
AigLit SubstituteBase::subs(AigLit fla) {
    vector<AigLit> todo;
    todo.push_back(fla);
    AigLit tmp;
    while (!todo.empty()) {
        const AigLit t = todo.back();
        todo.pop_back();
        if (subs_node(t, tmp)) continue;
        assert(t.is_node());
        const auto ptr = t.ptr();
        const auto n = factory.get_node(ptr);
        AigLit ares, bres;
        const auto ac = subs_node(n.a(), ares);
        const auto bc = subs_node(n.b(), bres);
        if (ac && bc) {
            const bool changed = !n.a().equals(ares) || !n.b().equals(bres);
            const AigLit new_val = changed ? factory.mk_and(ares, bres) : AigLit::mk_node(ptr, false);
            insert_chk(node_cache, ptr, new_val);
        } else {
            todo.push_back(t);
            todo.push_back(n.a());
            todo.push_back(n.b());
        }

    }
    if (!subs_node(fla, tmp)) assert(0);
    return tmp;
}

bool Substitute::subs_node(const AigLit& t, AigLit& res) {
    if (t.is_var()) {
        const auto v = t.var();
        const auto val = eval(v, s);
        if (val == l_Undef) res = t;
        else res = t.sign() == (val == l_False) ? factory.mk_true() : factory.mk_false();
        return true;
    }
    if (t.is_node()) {
        const auto ptr = t.ptr();
        const auto i = node_cache.find(ptr);
        if (i == node_cache.end()) return false;
        res = t.sign() ? factory.neg(i->second) : i->second;
        return true;
    }
    assert(factory.is_true(t) || factory.is_false(t));
    res = t;
    return true;
}

bool SubstituteAndFreshen::subs_node(const AigLit& t, AigLit& res) {
    if (t.is_var()) {
        const auto v = t.var();
        const auto val = eval(v, s);
        if (val == l_Undef) {
            auto nv = mapped(v);
            if (nv == FRESHEN) {
                nv = fvars.mk_fresh();
                map_to(v, nv);
            }
            res = factory.mk_var(nv, t.sign());
        } else {
            res = t.sign() == (val == l_False) ? factory.mk_true() : factory.mk_false();
        }
        return true;
    }
    return Substitute::subs_node(t,res);
}


bool SubstituteAndFreshenMap::subs_node(const AigLit& t, AigLit& res) {
    if (t.is_var()) {
        const auto v = t.var();
        const auto i = s.find(v);
        if (i != s.end()) {
            const auto& nval = i->second;
            res = t.sign() ? factory.neg(nval) : nval;
        } else {
            auto nv = mapped(v);
            if (nv == FRESHEN) {
                nv = fvars.mk_fresh();
                map_to(v, nv);
            }
            res = factory.mk_var(nv, t.sign());
        }
        return true;
    }
    if (t.is_node()) {
        const auto ptr = t.ptr();
        const auto i = node_cache.find(ptr);
        if (i == node_cache.end()) return false;
        res = t.sign() ? factory.neg(i->second) : i->second;
        return true;
    }
    assert(factory.is_true(t) || factory.is_false(t));
    res = t;
    return true;
}

void Substitute::mark_used(AigLit l, VarSet& vs) {
    std::unordered_set<size_t> seen;
    vector<AigLit> todo;
    AigLit ares, bres;
    todo.push_back(l);
    while (!todo.empty()) {
        const AigLit t = todo.back();
        todo.pop_back();
        //factory.print(std::cerr<<__LINE__<<endl, t)<<endl;
        if (t.is_var()) {
            const auto v = t.var();
            if (eval(v, s) != l_Undef) vs.add(v);
        } else if (t.is_node()) {
            if (mark(seen, t.ptr())) {
                const auto n = factory.get_node(t.ptr());
                const auto ac = subs_node(n.a(), ares);
                const auto bc = subs_node(n.b(), bres);
                const bool af = factory.is_false(ares);
                const bool bf = factory.is_false(bres);
                VERIFY(ac && bc);
                if (af || !bf) todo.push_back(n.a());
                if (!af) todo.push_back(n.b());
            }
        }
    }
}
