/*
 * File:  level_solver.cpp
 * Author:  mikolas
 * Created on:  10 Jan 2017 19:33:54
 * Copyright (C) 2017, Mikolas Janota
 */
#include "level_solver.h"
#include "cover_simplifier.h"
#include <algorithm>
#include <cstdlib>
using SATSPC::lit_Undef;
using SATSPC::LSet;
using std::endl;
using std::make_pair;
using std::pair;
using std::unordered_map;
using std::unordered_set;

LevelSolver::LevelSolver(const Options &_options, VarManager &vm,
                         AigFactory &_factory, const LevelInfo &li,
                         const ProximityTable &prox, Level qlevel)
    : options(_options), verb(options.get_verbose()),
      randomize(options.get_rndmodel()), var_mng(vm), level_info(li.prefix()),
      factory(_factory), prox(prox), sat(new SATSOLVER()),
      encoder(vm, qlevel, factory, *sat), util(factory), qlevel(qlevel),
      qtype(level_info.level_type(qlevel)), refine_counter(0),
      frefine_counter(0),
      abstraction(qtype == EXISTENTIAL ? factory.mk_true()
                                       : factory.mk_false()) {
    encoder.alloc_var(level_info.maxv());
}

LevelSolver::~LevelSolver() { delete sat; }

void LevelSolver::strenghten(const AigLit f) {
    LOG(5, factory.print(OUT << "strenghten:" << qtype << qlevel << endl, f, 2)
               << endl;);
    const auto ef = encoder(f);
    switch (qtype) {
    case UNIVERSAL:
        encoder.add(~ef);
        abstraction = factory.mk_or(abstraction, f);
        break;
    case EXISTENTIAL:
        encoder.add(ef);
        abstraction = factory.mk_and(abstraction, f);
        break;
    }
    LOG(5, factory.print(OUT << "abstraction:" << qtype << qlevel << endl,
                         abstraction, 2)
               << endl;);
}

AigLit LevelSolver::get_input(SATSPC::LSet &core, const VarSet &domain) {
    auto remaining = domain.size();
    vector<AigLit> tmp;
    tmp.reserve(domain.size());
    for (int i = 0; remaining > 0 && i < core.size(); ++i) {
        const Lit l = core[i];
        const Var v = var(l);
        if (!domain.get(v))
            continue;
        --remaining;
        tmp.push_back(factory.mk_var(v, sign(l)));
    }
    return util.and_(tmp, false);
}

void LevelSolver::make_blocking_clause(const SATSPC::LSet &core,
                                       vec<Lit> &blocking_clause) {
    int i = core.size();
    while (i--) {
        const Lit l = core[i];
        const bool reduce = level_info.qlev(l) > qlevel;
        assert(level_info.type(l) != qtype || !reduce);
        if (!reduce)
            blocking_clause.push(l);
    }
}

size_t log2(size_t x) {
    size_t retv = 0;
    while (x > 0) {
        x /= 2;
        ++retv;
    }
    return retv;
}

bool gt2nd(const pair<size_t, size_t> &a, const pair<size_t, size_t> &b) {
    return (a.second > b.second);
}

bool is_opposite(const Lit &l1, const Lit &l2) {
    if (l1 == lit_Undef || l2 == lit_Undef)
        return false;
    assert(var(l1) == var(l2));
    return sign(l1) != sign(l2);
}

void LevelSolver::diff(Var target_variable, const Core &a, const Core &b,
                       unordered_set<Var> &vs) const {
    for (Lit l : a) {
        const Var v = var(l);
        if (level_info.depends_on(target_variable, v) &&
            is_opposite(l, get_vlit(v, b)))
            vs.insert(v);
    }
}

void LevelSolver::get_implicants(Var target_variable, Level strat_level,
                                 VarSet &domain, vector<LSet *> &trues,
                                 vector<LSet *> &falses) {
    if (verb > 5)
        OUT << "get_implicants: " << target_variable << endl;
    vector<size_t> freqsn;
    unordered_set<Var> vs;
    for (size_t ia = 0; ia < cores.size(); ++ia) {
        auto &coreA = cores[ia];
        const Lit vlitA = get_vlit(target_variable, coreA);
        if (vlitA == lit_Undef)
            continue;
        for (size_t ib = ia + 1; ib < cores.size(); ++ib) {
            auto &coreB = cores[ib];
            const Lit vlitB = get_vlit(target_variable, coreB);
            if (vlitB == lit_Undef || vlitB == vlitA)
                continue;
            vs.clear();
            diff(target_variable, coreA, coreB, vs);
            diff(target_variable, coreB, coreA, vs);
            for (Var dv : vs) {
                const auto dvix = (size_t)dv;
                if (freqsn.size() <= dvix)
                    freqsn.resize(dvix + 1, 0);
                (freqsn[dvix])++;
            }
        }
    }
    if (options.get_proximity()) {
        const auto i = prox.table().find(target_variable);
        LOG_LRN(lout << "PROX: " << target_variable << ":";);
        if (i != prox.table().end()) {
            for (ProximityTable::Val val : *(i->second)) {
                assert(level_info.depends_on(target_variable, val.first));
                const auto dvix = (size_t)val.first;
                const size_t inc = (size_t)(val.second * 64);
                if (!inc)
                    continue;
                if (freqsn.size() <= dvix)
                    freqsn.resize(dvix + 1, 0);
                LOG_LRN(lout << " " << val.first << ":" << val.second << ":"
                             << freqsn[dvix] << "+" << inc;);
                freqsn[dvix] += inc;
            }
        }
        LOG_LRN(lout << endl;);
    }

    vector<pair<size_t, size_t>> ordering;
    for (size_t vx = 0; vx < freqsn.size(); ++vx) {
        auto &f = freqsn[vx];
        if (f)
            ordering.push_back(make_pair(vx, f));
    }
    std::sort(ordering.begin(), ordering.end(), gt2nd);
    LOG_LRN(lout << "ORD: "; for (auto i
                                  : ordering) lout
                             << " " << i.first << ":" << i.second;
            lout << endl;);
    if (verb > 5) {
        OUT << "freqsn: " << endl;
        for (size_t vx = 0; vx < freqsn.size(); ++vx)
            OUT << vx << ":" << freqsn[vx] << endl;
        OUT << endl;
        OUT << "ordering: " << endl;
        for (auto o : ordering)
            OUT << o.first << " : " << o.second << endl;
        OUT << endl;
    }

    vector<LSet *> implicants(cores.size());
    for (size_t i = 0; i < cores.size(); i++)
        implicants[i] = new LSet();

    for (size_t ia = 0; ia < cores.size(); ia++) {
        auto &coreA = cores[ia];
        LOG(5, prn_sample(OUT << "coreA ", coreA, strat_level, 0) << endl;);
        const Lit vlitA = get_vlit(target_variable, coreA);
        if (vlitA == lit_Undef)
            continue;
        (sign(vlitA) ? falses : trues).push_back(implicants[ia]);
        for (size_t ib = ia + 1; ib < cores.size(); ib++) {
            auto &coreB = cores[ib];
            const Lit vlitB = get_vlit(target_variable, coreB);
            if (vlitB == lit_Undef || vlitA == vlitB)
                continue;
            LOG(5, prn_sample(OUT << "coreA ", coreB, strat_level, 0) << endl;);
            Lit diff_lit = lit_Undef;
            // for (Var domv : domain) {// try to reuse lits already seen
            for (auto di = domain.ord_begin(); di != domain.ord_end(); ++di) {
                const auto domv = *di;
                const Lit domv_litA = get_vlit(domv, coreA);
                const Lit domv_litB = get_vlit(domv, coreB);
                if (is_opposite(domv_litA, domv_litB)) {
                    diff_lit = domv_litA;
                    break;
                }
            }
            for (size_t j = 0; diff_lit == lit_Undef && j < ordering.size();
                 j++) {
                const Var cand_v = (Var)(ordering[j].first);
                const Lit candv_litA = get_vlit(cand_v, coreA);
                const Lit candv_litB = get_vlit(cand_v, coreB);
                if (is_opposite(candv_litA, candv_litB))
                    diff_lit = candv_litA;
            }
            /* LOG(6, OUT << "diff_lit " << diff_lit << endl;); */
            if (diff_lit != lit_Undef) {
                assert(level_info.depends_on(target_variable, var(diff_lit)));
                domain.add(var(diff_lit));
                implicants[ia]->insert(diff_lit);
                implicants[ib]->insert(~diff_lit);
            }
        }
    }
}

#if 0
void LevelSolver::get_domain(Var v, VarSet& domain, Level strat_level) {
    if(verb>5) OUT<<"get_domain: " << v << endl;
    vector<size_t> freqsn;
    unordered_set<Var> vs;
    for (size_t ia = 0; ia < cores.size(); ++ia) {
        auto& coreA = *(cores[ia]);
        const Lit vlitA = get_vlit(v, coreA);
        if (vlitA == lit_Undef) continue;
        for (size_t ib = ia+1; ib < cores.size(); ++ib) {
            auto& coreB = *(cores[ib]);
            const Lit vlitB = get_vlit(v, coreB);
            if (vlitB == lit_Undef || vlitB == vlitA) continue;
            vs.clear();
            for (int j = 0; j < coreA.size(); j++) {
                const Lit l = coreA[j];
                const Var lv = var(l);
                if (level_info.qlev(lv) >= strat_level) continue;
                if (l != get_vlit(lv, coreB)) vs.insert(lv);
            }
            for (int j = 0; j < coreB.size(); j++) {
                const Lit l = coreB[j];
                const Var lv = var(l);
                if (level_info.qlev(lv) >= strat_level) continue;
                if (l != get_vlit(lv, coreA)) vs.insert(lv);
            }
            for (Var dv : vs) {
                const auto dvix = (size_t)dv;
                if (freqsn.size() <= dvix) freqsn.resize(dvix+1,0);
                (freqsn[dvix])++;
            }
        }
    }
    vector<pair<size_t,size_t> > ordering;
    for (size_t vx = 0; vx < freqsn.size(); ++vx) {
        auto& f = freqsn[vx];
        if (f) ordering.push_back(make_pair(vx, f));
    }
    std::sort(ordering.begin(),ordering.end(), gt2nd);
    if(verb>5){
        OUT<<"freqsn: " << endl;
        for (size_t vx = 0; vx < freqsn.size(); ++vx)  OUT<< vx << ":" << freqsn[vx] << endl;
        OUT<< endl;
        OUT<<"ordering: " << endl;
        for (auto o : ordering) OUT<< o.first << " : " << o.second << endl;
        OUT<< endl;
    }


    if (ordering.empty()) return;

    LitBitSet ls;

    for (size_t ia = 0; ia < cores.size(); ia++) {
        auto& coreA = *(cores[ia]);
        LOG(4+1, OUT<<"coreA "<< v << " : " << coreA << endl;);
        const Lit vlitA = get_vlit(v, coreA);
        if (vlitA == lit_Undef) continue;
        for (int j = 0; j < coreA.size(); j++) {
            const Lit l = coreA[j];
            if (level_info.qlev(l) >= strat_level) continue;
            ls.add(l);
        }
        for (size_t ib = ia + 1; ib < cores.size(); ib++) {
            auto& coreB = *(cores[ib]);
            LOG(4+1, OUT<<"coreB "<< v << " : " << coreB << endl;);
            const Lit vlitB = get_vlit(v, coreB);
            if (vlitB == lit_Undef || vlitA == vlitB) continue;
            Lit diff_lit = lit_Undef;
            for (Var domv : domain) {
                const Lit domv_litA = get_vlit(domv, coreA);
                const Lit domv_litB = get_vlit(domv, coreB);
                if (domv_litA != domv_litB) {
                    diff_lit = domv_litA;
                    break;
                }
            }
            for (size_t j = 0; diff_lit == lit_Undef && j < ordering.size(); j++) {
                const Var cand_v = (Var)(ordering[j].first);
                const Lit candv_litA = get_vlit(cand_v, coreA);
                const Lit candv_litB = get_vlit(cand_v, coreB);
                if (candv_litA != candv_litB) diff_lit = candv_litA;
            }
            LOG(5+1, OUT<<"diff_lit "<< diff_lit << endl;);
            assert(diff_lit != lit_Undef);
            domain.add(var(diff_lit));
        }
    }
}

void LevelSolver::get_domain(Var v, VarSet& domain, Level strat_level) {
    LitBitSet ls;
    for (size_t ia = 0; ia < cores.size(); ia++) {
        auto& coreA = *(cores[ia]);
        LOG(4+1, OUT<<"coreA "<< v << " : " << coreA << endl;);
        const Lit vlitA = get_vlit(v, coreA);
        if (vlitA == lit_Undef) continue;
        for (int j = 0; j < coreA.size(); j++) {
            const Lit l = coreA[j];
            if (level_info.qlev(l) >= strat_level) continue;
            ls.add(l);
        }
        for (size_t ib = ia + 1; ib < cores.size(); ib++) {
            auto& coreB = *(cores[ib]);
            LOG(4+1, OUT<<"coreB "<< v << " : " << coreB << endl;);
            const Lit vlitB = get_vlit(v, coreB);
            if (vlitB == lit_Undef || vlitA == vlitB) continue;
            Lit diff_lit = lit_Undef;
            for (Var domv : domain) {
              const Lit domv_litA = get_vlit(domv, coreA);
              const Lit domv_litB = get_vlit(domv, coreB);
              if (domv_litA != domv_litB) {
                diff_lit = domv_litA;
                break;
              }
            }
            for (int j = 0; diff_lit == lit_Undef && j < coreB.size(); j++) {
                const Lit l = coreB[j];
                if (level_info.qlev(l) >= strat_level) continue;
                if (ls.get(l)) continue;
                diff_lit = l;
            }
            LOG(5+1, OUT<<"diff_lit "<< diff_lit << endl;);
            if (diff_lit == lit_Undef) {
                for (int j = 0; j < coreA.size(); j++) {
                    const Lit l = coreA[j];
                    if (level_info.qlev(l) >= strat_level) continue;
                    if (coreB.has(l)) continue;
                    diff_lit = l;
                    break;
                }
            }
            assert(diff_lit != lit_Undef);
            domain.add(var(diff_lit));
        }
    }
}
#endif

Var LevelSolver::fresh_var(Level ql) {
    const auto retv = encoder.fresh();
    level_info.add_var(retv, ql);
    return retv;
}

bool LevelSolver::get_subs(unordered_map<size_t, AigLit> &node_cache,
                           unordered_map<Var, AigLit> &subs, const AigLit &t,
                           AigLit &res) {
    if (t.is_var()) {
        const auto v = t.var();
        const auto j = subs.find(v);
        if (j != subs.end())
            res = t.sign() ? factory.neg(j->second) : j->second;
        else if (level_info.qlev(v) > qlevel) {
            const auto fv = factory.mk_var(var_mng.fresh_var(qlevel), false);
            insert_chk(subs, j, v, fv);
            res = t.sign() ? factory.neg(fv) : fv;
        } else {
            res = t;
        }
        return true;
    }
    if (t.is_node()) {
        const auto ptr = t.ptr();
        const auto i = node_cache.find(ptr);
        if (i == node_cache.end())
            return false;
        res = t.sign() ? factory.neg(i->second) : i->second;
        return true;
    }
    assert(factory.is_true(t) || factory.is_false(t));
    res = t;
    return true;
}

AigLit LevelSolver::subs(const unordered_map<Var, AigLit> &strats, AigLit fla) {
    LOG(5, factory.print(OUT << "subs in ", fla) << endl;);
    unordered_map<Var, AigLit> subs = strats;
    vector<AigLit> todo;
    todo.push_back(fla);
    unordered_map<size_t, AigLit> node_cache;
    AigLit tmp;
    // std::cerr<<"node count bf: " << factory.node_count()<<endl;
    while (!todo.empty()) {
        const AigLit t = todo.back();
        todo.pop_back();
        if (get_subs(node_cache, subs, t, tmp))
            continue;
        assert(t.is_node());
        const auto ptr = t.ptr();
        const auto n = factory.get_node(ptr);
        AigLit ares, bres;
        const auto ac = get_subs(node_cache, subs, n.a(), ares);
        const auto bc = get_subs(node_cache, subs, n.b(), bres);
        if (ac && bc) {
            const bool changed = !n.a().equals(ares) || !n.b().equals(bres);
            const AigLit new_val = changed ? factory.mk_and(ares, bres)
                                           : AigLit::mk_node(ptr, false);
            insert_chk(node_cache, ptr, new_val);
        } else {
            todo.push_back(t);
            todo.push_back(n.a());
            todo.push_back(n.b());
        }
    }
    // std::cerr<<"node count af: " << factory.node_count()<<endl;
    if (!get_subs(node_cache, subs, fla, tmp))
        assert(0);
    LOG(5, factory.print(OUT << "result subs", tmp) << endl;);
    return tmp;
}

ostream &LevelSolver::prn_sample(ostream &o, Core &core, const VarSet &domain,
                                 Var v, size_t limit) {
    bool f = true;
    const bool use_limit = limit > 0;
    for (Var dv : domain) {
        const Lit l = get_vlit(dv, core);
        if (l == lit_Undef)
            continue;
        if (f)
            f = false;
        else
            o << " ";
        --limit;
        if (!use_limit || limit--) {
            o << l;
        } else {
            o << "...";
            break;
        }
    }
    return o << " | " << get_vlit(v, core);
}

ostream &LevelSolver::prn_sample(ostream &o, const Core &core,
                                 Level strat_level, size_t _limit) {
    bool f = true;
    auto i = core.size();
    const bool use_limit = _limit > 0;
    size_t limit = _limit;
    while (i--) {
        const Lit l = core[i];
        if (level_info.qlev(l) >= strat_level)
            continue;
        if (f)
            f = false;
        else
            o << " ";
        o << l;
        if (use_limit && !limit) {
            o << "...";
            break;
        }
        if (use_limit && limit)
            --limit;
    }
    o << " |";
    i = core.size();
    limit = _limit;
    while (i--) {
        const Lit l = core[i];
        if (level_info.qlev(l) != strat_level)
            continue;
        o << " " << l;
        if (use_limit && !limit) {
            o << "...";
            break;
        }
        if (use_limit && limit)
            --limit;
    }
    return o;
}

bool LevelSolver::magic(size_t refine_counter, Level strat_level) {
    if (!options.get_learn())
        return false;
    const auto vcount = level_info.level_vars(strat_level).size();
    if (refine_counter == 1)
        return true;
    if (refine_counter < 16)
        return false;
    if (vcount == 0)
        return false;
    if (options.get_cyclic()) {
        // const auto r = refine_counter % 56;
        // return r==4 || r==8 || r==24 || r==0;
        const auto r = refine_counter % 40;
        return r == 2 || r == 4 || r == 6 || r == 8 || r == 12 || r == 16 ||
               r == 24 || r == 0;
    } else {
        if (vcount == 1 && refine_counter == 2)
            return true;
        if (1 < vcount && vcount <= 5 &&
            refine_counter == ((size_t)1 << (vcount - 1)))
            return true;
        return (refine_counter % 64) == 0;
    }
}

bool LevelSolver::update_strategy(const AigLit &s, Var v, vector<Core> cores,
                                  AigLit &updated) {
    vector<Core> lacks_response;
    LOG(6, prn_strat(OUT << "update_strategy ", v, s););
    for (auto core : cores)
        if (!is_good_strategy(core, v, s, lacks_response))
            return false;
    updated = s;
    return lacks_response.empty();
}

bool LevelSolver::is_good_strategy(const Core &core, Var v, const AigLit &s,
                                   vector<Core> &lacks_response) {
    const Lit r = get_vlit(v, core);
    if (r == lit_Undef)
        return true;
    assert(var(r) == v);
    const lbool val = util.eval(s, core);
    LOG(6, {
        prn_strat(OUT << "is_good_strategy " << level_info.type(v)
                      << level_info.qlev(v) << " ",
                  v, s);
        prn_sample(OUT << " core: ", core, level_info.qlev(v), 0) << endl;
        OUT << " val: " << val << endl;
    });
    if (val == l_Undef) {
        lacks_response.push_back(core);
        return true;
    }
    return (val == l_False) == sign(r);
}

ostream &LevelSolver::prn_strat(ostream &o, Var v, AigLit strategy,
                                const char *const comment) {
    o << "strat (" << comment << ") : " << v << ": [";
    bool nl = false;
    if (strategy.is_node()) {
        const auto n = factory.get_node(strategy);
        if (n.a().is_node() || n.b().is_node())
            nl = true;
    }
    if (nl)
        o << endl;
    else
        o << " ";
    return factory.print(o, strategy, nl ? 2 : 0) << " ] " << endl;
}

void LevelSolver::learn(Level strat_level) {
    LOG_LRN({
        lout << "CORES:" << endl;
        for (auto cr : cores)
            prn_sample(lout << "  ", cr, strat_level, 0) << endl;
        lout << "END CORES" << endl;
    });
    LOG(5, {
        OUT << "cores:" << endl;
        for (auto cr : cores)
            prn_sample(OUT << " ", cr, strat_level) << endl;
    });

    if (!options.get_accum())
        strategies.clear();

    vector<AigLit> tmp;
    for (Var v : level_info.level_vars(strat_level)) {
        LOG(4, OUT << "processing " << v << endl;);
        const auto osi = strategies.find(v);
        AigLit strategy;
        if (osi == strategies.end() ||
            !update_strategy(osi->second, v, cores, strategy)) {
            VarSet domain;
            vector<LSet *> trues, falses;
            get_implicants(v, strat_level, domain, trues, falses);
            LOG(4, OUT << "domain " << v << " : " << domain << endl;);
#if 0
            const auto pl = mkLit(v);
            tmp.clear();
            for (LSet * c : cores) if (c->has(pl)) tmp.push_back(get_input(*c, domain));
            strategy = util.or_(tmp, true);
#else
            CoverSimplier cs(factory, v, domain, trues, falses);
            strategy = cs.run();
#endif
            cleanup(trues);
            cleanup(falses);
            LOG_LRN(lout << "domain : " << v << " : " << domain << endl;
                    // for (auto cr : cores) prn_sample(std::cerr  << " ",
                    // (*cr), domain, v, 0) << endl;
                    prn_strat(lout, v, strategy, "new"););
            if (osi == strategies.end())
                insert_chk(strategies, osi, v, strategy);
            else
                osi->second = strategy;
        } else {
            LOG_LRN(prn_strat(lout, v, strategy, "keeping"););
        }
        LOG(4, prn_strat(lout, v, strategy););
    }
}

void LevelSolver::add_core(const SATSPC::LSet &core) {
    cores.push_back(Core(core, true));
}

void LevelSolver::refine_fancy(const SATSPC::LSet &core, Level strat_level,
                               AigLit fla) {
    add_core(core);
    if (!magic(refine_counter, strat_level))
        return;
    LOG_LRN(lout << "LEVEL: " << qtype << qlevel << endl;);
    LOG_LRN(lout << "refine_counter: " << refine_counter << endl;);
    ++frefine_counter;
    learn(strat_level);
    __TIMECODE(AigLit ref = subs(strategies, fla););
    strenghten(ref);
    cleanup_cores();
    LOG_LRN(lout << "END LEVEL: " << qtype << qlevel << endl;);
}

void LevelSolver::block(const SATSPC::LSet &core) {
    vec<Lit> blocking_clause;
    make_blocking_clause(core, blocking_clause);
    encoder.add_(blocking_clause);
    LOG(4, OUT << "blocking:" << blocking_clause << endl;);
}

void LevelSolver::refine(const SATSPC::LSet &core, AigLit fla) {
    LOG(4, OUT << "refine " << qtype << qlevel << endl;);
    LOG(4, OUT << "refine core:" << core << endl;);
    ++refine_counter;
    block(core);
    __TIMECODE(refine_fancy(core, qlevel + 1, fla););
}

bool LevelSolver::solve(const vec<Lit> &assumptions) {
    __TIMECODE(const bool retv = sat->solve(assumptions););
    LOG(5, OUT << "solver " << &sat << " retv: " << retv << std::endl;);
    switch (randomize) {
    case 0: break;
    case 1: {
        const int coeff = 33;
        for (Var v : level_info.level_vars(qlevel))
            if ((rand() % 100) < coeff)
                sat->setPolarity(v, rand() & 1 ? l_True : l_False);
    } break;
    case 2: {
        const int coeff = 100;
        for (Var v = 0; v < sat->nVars(); ++v)
            if ((rand() % 100) < coeff)
                sat->setPolarity(v, rand() & 1 ? l_True : l_False);
    } break;
    default: assert(0);
    }
    return retv;
}

lbool LevelSolver::get_val(Var v) const { return eval(v, sat->get_model()); }
