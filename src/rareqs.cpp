/*
 * File:  rareqs.cpp
 * Author:  mikolas
 * Created on:  06 Apr 2017 17:56:50
 * Copyright (C) 2017, Mikolas Janota
 */
#include "rareqs.h"
#include "auxiliary.h"
#include "dec_tree.h"
#include "encoder.h"
#include "game_cleanup.h"
#include "sat_interface.h"
#include "sat_varmanager.h"
#include "substitute.h"
#include <algorithm>
#include <memory>
#include <ostream> // for operator<<, ostream
#include <string>
#include <unordered_map> // for unordered_map

using namespace SATSPC;
using std::endl;
using std::max;
using std::unique_ptr;

ostream &Rareqs::prn_strat(ostream &o, Var v, AigLit strategy,
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
    return factory.print_fancy(o, strategy, dbg_off + (nl ? 2 : 0))
           << " ] " << endl;
}

size_t Rareqs::sum_ref_counter = 0;
double Rareqs::learning_time = 0.0;

Rareqs::Rareqs(QuantifierType qt, const Options &options, AigFactory &factory)
    : RareqsBase(options, qt), block(options.get_blocking()),
      block_level(
          block ? std::max(static_cast<size_t>(4), options.get_blocking_arg())
                : -1),
      factory(factory), util(factory), ref_counter(0),
      learning_interval(options.get_interval() ? options.get_interval_arg()
                                               : 16) {
    games.prop = factory.mk_true();
}

Rareqs::~Rareqs() {}

void Rareqs::get_move(Move &out_move) { move.copyTo(out_move); }
void Rareqs::get_move(Move &out_move, const vector<Var> &vs) {
    set(vs, move, out_move);
}

bool Rareqs::magic() {
    if (!options.get_learn())
        return false;
    return (ref_counter % learning_interval) == 0;
}

void Rareqs::refine(std::unique_ptr<RareqsBase> &abstraction_solver,
                    size_t game_index, const Move &candidate,
                    const Move &counter_move) {
    ++ref_counter;
    ++sum_ref_counter;
    if (0 && ref_counter > 300) {
        std::cerr << "KILL" << endl;
        exit(0);
    }
    const Game &g = games.gs[game_index];
    LOG(3, out() << "refine " << ref_counter << endl;);
    LOG(4, print_model(out() << "cex:", counter_move, g.p[0].second) << endl;);
    LOG(4, factory.print_fancy(out() << "losing game:" << g.p << "(" << endl,
                               g.m, dbg_off)
               << ")" << endl;);
    multi_plays[game_index].push_back(
        Play(mk_core(candidate, free), mk_core(counter_move, g.p[0].second)));
    if (magic()) {
        LOG(3, out() << "refine fancy" << endl;);
        refine_fancy(abstraction_solver, game_index);
        multi_plays[game_index].clear();
    } else {
        refine_trad(abstraction_solver, g, counter_move);
    }
}

void Rareqs::refine_fancy(std::unique_ptr<RareqsBase> &abstraction_solver,
                          size_t game_index) {
    const Game &g = games.gs[game_index];
    const auto &ps = multi_plays[game_index];
    DecTree dt(factory);
    for (const auto &p : ps)
        dt.add_sample(p);

    VarSet free_set;
    free_set.add_all(free);
    if (!options.get_accum())
        strategies.clear();
    LOG(6, out() << "strategies" << endl;);
    for (Var tv : g.p[0].second) {
        const auto osi = strategies.find(tv);
        const bool keep =
            osi != strategies.end() && is_good_strategy(tv, osi->second, ps);
        const auto _t0 = read_cpu_time();
        if (!keep)
            strategies[tv] = dt(tv, free_set);
        const auto _t1 = read_cpu_time();
        learning_time += _t1 - _t0;
        LOG(6, prn_strat(out(), tv, strategies[tv], keep ? "keep" : "new"););
    }
    LOG(6, {
        out() << "samples" << endl;
        for (const auto &p : ps)
            out() << p.me << " | " << p.opp << endl;
    });
    const bool freshen = g.p.size() > 1;
    unique_ptr<SubstituteAndFreshenMap> sub(
        new SubstituteAndFreshenMap(factory, strategies, fresh));
    if (freshen)
        for (Var v : g.p[1].second)
            sub->mark(v);
    Game refinement;
    refinement.m = (*sub)(g.m);
    for (size_t i = 2; i < g.p.size(); ++i)
        refinement.p.push_back(g.p[i]);
    if (freshen) {
        Var nv;
        for (Var v : g.p[1].second)
            if (sub->was_freshened(v, nv)) {
                fresh_vars.push_back(nv);
                abstraction_solver->add_free(nv);
            }
    }
    abstraction_solver->add_game(refinement);
}

void Rareqs::refine_trad(std::unique_ptr<RareqsBase> &abstraction_solver,
                         const Game &g, const Move &counter_move) {
    const bool freshen = g.p.size() > 1;
    unique_ptr<SubstituteAndFreshen> sub(
        new SubstituteAndFreshen(factory, counter_move, fresh));
    if (freshen)
        for (Var v : g.p[1].second)
            sub->mark(v);
    Game refinement;
    refinement.m = (*sub)(g.m);
    for (size_t i = 2; i < g.p.size(); ++i)
        refinement.p.push_back(g.p[i]);
    if (freshen) {
        Var nv;
        for (Var v : g.p[1].second)
            if (sub->was_freshened(v, nv)) {
                fresh_vars.push_back(nv);
                abstraction_solver->add_free(nv);
            }
    }
    abstraction_solver->add_game(refinement);
}

Rareqs *Rareqs::make_solver(const Options &options, AigFactory &fc,
                            const Prefix &pref, AigLit closed_mx, int off) {
    const bool empty = pref.empty();
    // LOG(4, fc.print(out()<<"make_solver:"<<pref<<endl, closed_mx, 0, false,
    // true)<<endl;);
    Rareqs *retv = new Rareqs(empty ? EXISTENTIAL : pref[0].first, options, fc);
    retv->set_dbg_offset(off);
    Game game;
    game.m = closed_mx;
    for (size_t i = 1; i < pref.size(); ++i)
        game.p.push_back(pref[i]);
    if (!empty)
        for (Var v : pref[0].second)
            retv->add_free(v);
    retv->add_game(game);
    return retv;
}

bool Rareqs::calc_counter_move(const Game &game, const Move &candidate,
                               Move &counter_move,
                               /*out*/ vector<Lit> &block_clause) {
    LOG(4, out() << "counter_move chk" << endl;);
    assert(!game.p.empty() && game.p[0].first == neg(qt));
    if (game.p.size() <= 1) {
        SATSOLVER s;
        SatVarManager svm(s);
        Encoder<SATSOLVER> e(svm, 0, factory, s);
        e.alloc_var(max(maxv(free), maxv(game.p[0].second)));
        const QuantifierType opponent = neg(qt);
        e.add(e(opponent == UNIVERSAL ? factory.neg(game.m) : game.m));
        vec<Lit> a;
        to_lits(free, candidate, a, true);
        const bool retv = s.solve(a);
        if (retv)
            set(game.p[0].second, s.get_model(), counter_move);
        /* TODO
        if (retv && should_block(game)) {
            block_clause.reserve(s.conflict.size());
            for (int i = 0; i < s.conflict.size(); ++i)
        block_clause.push_back(s.conflict[i]);
        }
            */
        return retv;
    }
    Substitute sub(factory, candidate);
    auto sm = sub(game.m);
    LOG(8, factory.print_fancy(out() << "sub in:" << endl, game.m, dbg_off)
               << endl;);
    LOG(8, factory.print_fancy(out() << "sub out:" << endl, sm, dbg_off)
               << endl;);
    unique_ptr<Rareqs> counter_solver(
        make_solver(options, factory, game.p, sm, dbg_off + 2));
    const bool opponent_wins = counter_solver->wins();
    // sum_ref_counter += counter_solver->get_sum_refine_counter();
    if (opponent_wins)
        counter_solver->get_move(counter_move);
    if (opponent_wins && should_block(game)) {
        VarSet used;
        sub.mark_used(game.m, used);
        LOG(8, out() << "used:" << used << endl;);
        for (Var v : free)
            if (used.get(v))
                block_clause.push_back(eval(v, candidate) == l_True ? ~mkLit(v)
                                                                    : mkLit(v));
        LOG(8, out() << "block_clause:" << block_clause << endl;);
    }
    return opponent_wins;
}

bool Rareqs::wins() {
    if (factory.is_false(games.prop))
        return false;
    if (games.gs.empty()) {
        SATSOLVER s;
        SatVarManager svm(s);
        Encoder<SATSOLVER> e(svm, 0, factory, s);
        LOG(4, factory.print_fancy(out() << "solving sat:" << endl, games.prop,
                                   dbg_off)
                   << endl;);
        e.alloc_var(maxv(free));
        e.add(e(games.prop));
        for (const auto &cl : games.clauses)
            e.add(cl);
        const bool retv = s.solve();
        if (retv)
            set(free, s.get_model(), move);
        return retv;
    }
    LOG(6, out() << "fresh:" << fresh << endl;);
    size_t ml = 0;
    for (const auto &g : games.gs)
        ml = std::max(ml, g.p.size());

    unique_ptr<RareqsBase> abstraction_solver(
        ml < 3 ? static_cast<RareqsBase *>(new RareqsSAT(qt, options, factory))
               : static_cast<RareqsBase *>(new Rareqs(qt, options, factory)));
    LOG(5, out() << "using incremental abstraction: "
                 << (abstraction_solver->is_incremental() ? "Y" : "N")
                 << endl;);
    abstraction_solver->set_dbg_offset(dbg_off + 2);
    for (Var v : free)
        abstraction_solver->add_free(v);
    abstraction_solver->strengthen(games.prop);
    for (LitSet &c : games.clauses)
        abstraction_solver->strengthen(c);
    Move candidate, cex;
    vector<Lit> block_clause;
    lbool retv = l_Undef;
    if (options.get_initial()) {
        cex.clear();
        const size_t il = options.get_initial_arg();
        for (size_t i = 0; i < games.gs.size(); ++i) {
            const auto &g = games.gs[i];
            if (g.p.size() < il || g.p[0].first == qt)
                continue;
            for (Var v : g.p[0].second)
                set(v, l_False, cex);
            refine_trad(abstraction_solver, g, cex);
            cex.clear();
        }
    }
    do {
        candidate.clear();
        cex.clear();
        if (!win_abstr(abstraction_solver, candidate)) {
            retv = l_False;
            break;
        }
        LOG(4, print_model(out() << "candidate:", candidate, free) << endl;);
        retv = l_True;
        for (size_t gi = 0; gi < games.gs.size(); ++gi) {
            const Game &g = games.gs[gi];
            block_clause.clear();
            if (calc_counter_move(g, candidate, cex, block_clause)) {
                retv = l_Undef;
                refine(abstraction_solver, gi, candidate, cex);
                if (should_block(g)) {
                    const LitSet bl = LitSet::mk(block_clause);
                    LOG(4, out() << "blocking:" << bl << endl;);
                    abstraction_solver->strengthen(bl);
                }
                break;
            }
        }
    } while (retv == l_Undef);
    if (retv == l_True)
        candidate.moveTo(move);
    while (!fresh_vars.empty()) {
        LOG(6, out() << "unmark:" << fresh_vars.back() << endl;);
        VERIFY(fresh.unmark(fresh_vars.back()));
        fresh_vars.pop_back();
    }
    for (auto &ps : multi_plays)
        ps.clear();
    LOG(6, out() << "fresh:" << fresh << endl;);
    LOG(4, out() << "retv:" << retv << endl;);
    return retv == l_True;
}

void Rareqs::add_free(Var v) {
    LOG(6, out() << "free:" << v << endl;);
    if (fresh.is_marked(v)) {
        out() << "ERR:" << v << endl;
        out() << "free:" << free << endl;
        out() << "fresh_vars:" << fresh_vars << endl;
        out() << "fresh:" << fresh << endl;
    }
    free.push_back(v);
    VERIFY(fresh.mark(v));
}

void Rareqs::strengthen(AigLit l) {
    games.prop = factory.mk_and(games.prop, l);
}
void Rareqs::strengthen(const LitSet &clause) {
    games.clauses.push_back(clause);
}

void Rareqs::add_game(const Game &_g) {
    LOG(4, factory.print_fancy(out() << "add_game:" << _g.p << "(" << endl,
                               _g.m, dbg_off)
               << ")" << endl;);

    if (_g.p.empty()) {
        games.prop = factory.mk_and(
            games.prop, qt == EXISTENTIAL ? _g.m : factory.neg(_g.m));
        return;
    }
    games.gs.push_back(_g);
    GameCleanup gc(factory);
    auto &g = games.gs.back();
    gc.run(g);
    if (g.p.empty()) {
        LOG(4, factory.print_fancy(
                   out() << "add_game simplified to prop:" << g.p << endl, g.m,
                   dbg_off)
                   << endl;);
        games.prop = factory.mk_and(games.prop,
                                    qt == EXISTENTIAL ? g.m : factory.neg(g.m));
        games.gs.pop_back();
        return;
    }
    assert(g.p[0].first == neg(qt));
    for (const Quantification &q : g.p)
        for (Var v : q.second)
            fresh.mark(v);
    multi_plays.resize(multi_plays.size() + 1);
    assert(multi_plays.size() == games.gs.size());
    LOG(4,
        factory.print_fancy(
            out() << "add_game simplified:" << g.p << "(" << endl, g.m, dbg_off)
            << ")" << endl;);
}

bool Rareqs::win_abstr(unique_ptr<RareqsBase> &abstraction_solver,
                       Move &candidate) {
    const bool retv = abstraction_solver->wins();
    // sum_ref_counter += abstraction_solver->get_sum_refine_counter();
    if (retv)
        abstraction_solver->get_move(candidate, free);
    if (abstraction_solver->is_incremental()) {
        for (Var v : abstraction_solver->fresh_var_stack()) {
            fresh.mark(v);
            fresh_vars.push_back(v);
        }
        abstraction_solver->clear_fresh_var_stack();
    }
    return retv;
}

bool Rareqs::is_good_strategy(Var v, const AigLit &s, const vector<Play> &ps) {
    for (const auto &p : ps)
        if (!is_good_strategy(v, s, p))
            return false;
    return true;
}

bool Rareqs::is_good_strategy(Var v, const AigLit &s, const Play &p) {
    const Lit r = get_vlit(v, p.opp);
    if (r == lit_Undef)
        return true;
    assert(var(r) == v);
    const lbool val = util.eval(s, p.me);
    return val != l_Undef && ((val == l_False) == sign(r));
}

void RareqsSAT::add_free(Var v) {
    LOG(6, out() << "free:" << v << endl;);
    free.push_back(v);
    VERIFY(fresh.mark(v));
    enc->alloc_var(maxv(free));
}

void RareqsSAT::add_game(const Game &g) {
    LOG(4, factory.print_fancy(out() << "add_game:" << g.p << "(" << endl, g.m,
                               dbg_off)
               << ")" << endl;);
    if (g.p.empty()) {
        const auto ge = enc->encode(qt == EXISTENTIAL ? g.m : factory.neg(g.m));
        enc->add(ge);
    } else {
        assert(0);
    }
}

RareqsSAT::RareqsSAT(QuantifierType qt, const Options &options, AigFactory &fac)
    : RareqsBase(options, qt), factory(fac) {
#if EXTERNAL_SAT
    if (!options.get_external_sat()) {
        std::cerr << "external solver needed (-X option)" << endl;
        exit(100);
    }
    s.sat_solver_path = options.get_external_sat_arg();
#endif
    enc.reset(new Encoder<SATSolver>(*this, 0, factory, s));
    if (options.get_seed()) {
#ifdef USE_MINISAT
        s.random_seed = options.get_seed_arg();
        s.rnd_init_act = true;
#endif
        // std::cerr<<"setting seed"<<s.random_seed<<std::endl;
    }
}

void RareqsSAT::strengthen(AigLit l) {
    const auto ge = enc->encode(l);
    enc->add(ge);
}

void RareqsSAT::strengthen(const LitSet &clause) { enc->add(clause); }

void RareqsSAT::get_move(Move &move) { get_move(move, free); }
void RareqsSAT::get_move(Move &move, const vector<Var> &vs) {
    set(vs, s.get_model(), move);
}
