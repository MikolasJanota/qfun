/*
 * File:  rareqs.h
 * Author:  mikolas
 * Created on:  06 Apr 2017 17:43:09
 * Copyright (C) 2017, Mikolas Janota
 */
#pragma once
#include "aig.h"
#include "aig_util.h"
#include "encoder.h"
#include "fresh_vars.h"
#include "options.h"
#include "play.h"
#include "qtypes.h"
#include "rareqs_types.h"
#include "sat_interface.h"
#if EXTERNAL_SAT
#include "satwrapper.h"
#endif
#include <memory>

class RareqsBase {
  public:
    RareqsBase(const Options &o, QuantifierType qt)
        : options(o), verb(o.get_verbose()), qt(qt), dbg_off(0) {}
    virtual ~RareqsBase() {}

    QuantifierType quantifier_type() const { return qt; }
    virtual bool wins() = 0;
    virtual void add_free(Var v) = 0;
    virtual void add_game(const Game &game) = 0;
    virtual void strengthen(AigLit p) = 0;
    virtual void strengthen(const LitSet &clause) = 0;
    virtual void get_move(Move &out_move) = 0;
    virtual void get_move(Move &out_move, const vector<Var> &vs) = 0;
    void set_dbg_offset(int v) { dbg_off = v; }
    virtual bool is_incremental() const = 0;
    virtual const vector<Var> &fresh_var_stack() = 0;
    virtual void clear_fresh_var_stack() = 0;
    virtual size_t get_refine_counter() const = 0;
    virtual size_t get_sum_refine_counter() const = 0;
    virtual const vector<Var> &get_free() const = 0;

  protected:
    const Options &options;
    const int verb;
    const QuantifierType qt;
    int dbg_off;
    std::ostream &out() {
        for (int i = dbg_off; i; --i)
            OUT << ' ';
        return OUT;
    }
};

class Rareqs : public RareqsBase {
  public:
    static Rareqs *make_solver(const Options &options, AigFactory &fc,
                               const Prefix &pref, AigLit closed_mx,
                               int off = 0);

  public:
    Rareqs(QuantifierType qt, const Options &options, AigFactory &factory);
    virtual ~Rareqs();
    virtual bool wins();
    virtual void add_free(Var v);
    virtual const vector<Var> &get_free() const { return free; }
    virtual void add_game(const Game &game);
    virtual void get_move(Move &move);
    virtual void get_move(Move &out_move, const vector<Var> &vs);
    virtual bool is_incremental() const { return false; }
    virtual size_t get_refine_counter() const { return ref_counter; }
    virtual size_t get_sum_refine_counter() const { return sum_ref_counter; }
    virtual double get_learning_time() const { return learning_time; }

  public:
    virtual const vector<Var> &fresh_var_stack() {
        assert(0);
        return fresh_vars;
    }
    virtual void clear_fresh_var_stack() { assert(0); }
    virtual void strengthen(const LitSet &clause);

  protected:
    const bool block;
    const size_t block_level;
    AigFactory &factory;
    AigUtil util;
    MultiGame games;
    vector<Var> free;
    FreshVars fresh;
    vector<Var> fresh_vars;
    Move move;
    vector<vector<Play>> multi_plays;
    size_t ref_counter;
    static size_t sum_ref_counter;
    static double learning_time;
    const int learning_interval;
    std::unordered_map<Var, AigLit> strategies;
    bool calc_counter_move(const Game &game, const Move &candidate,
                           /*out*/ Move &counter_move,
                           /*out*/ vector<Lit> &block_clause);
    bool win_abstr(std::unique_ptr<RareqsBase> &abstraction_solver,
                   Move &candidate);
    void refine(std::unique_ptr<RareqsBase> &abstraction_solver,
                size_t game_index, const Move &move, const Move &cm);
    void refine_trad(std::unique_ptr<RareqsBase> &abstraction_solver,
                     const Game &game, const Move &cm);
    void refine_fancy(std::unique_ptr<RareqsBase> &abstraction_solver,
                      size_t game_index);
    bool magic();
    virtual void strengthen(AigLit l);
    Core mk_core(const Move &m, const vector<Var> &vs) { return Core(m, vs); }
    std::ostream &prn_strat(std::ostream &o, Var v, AigLit strategy,
                            const char *const comment);
    bool is_good_strategy(Var v, const AigLit &s, const vector<Play> &ps);
    bool is_good_strategy(Var v, const AigLit &s, const Play &p);
    inline bool should_block(const Game &g) const {
        return block && block_level <= g.p.size();
    }
};

class RareqsSAT : public RareqsBase, public VarManager {
  public:
#if EXTERNAL_SAT
    typedef SATSPC::SatWrapper SATSolver;
#else
    typedef SATSOLVER SATSolver;
#endif
    RareqsSAT(QuantifierType qt, const Options &options, AigFactory &fac);
    virtual ~RareqsSAT() {}
    virtual bool wins() { return s.solve(); }
    virtual void add_free(Var v);
    virtual const vector<Var> &get_free() const { return free; }
    virtual void add_game(const Game &game);
    virtual void strengthen(AigLit l);
    virtual void strengthen(const LitSet &clause);
    virtual void get_move(Move &move);
    virtual void get_move(Move &out_move, const vector<Var> &vs);
    void set_dbg_offset(int v) { dbg_off = v; }
    virtual bool is_incremental() const { return true; }
    virtual const vector<Var> &fresh_var_stack() { return fresh_vars; }
    virtual void clear_fresh_var_stack() { return fresh_vars.clear(); }
    virtual size_t get_refine_counter() const { return 0; }
    virtual size_t get_sum_refine_counter() const { return 0; }

  public:
    virtual void alloc_var(Var v, Level) { // from VarManager
        fresh.mark(v);
        s.new_variables(v);
    }
    virtual Var fresh_var(Level) { // from VarManager
        const Var retv = fresh.mk_fresh();
        s.new_variables(retv);
        fresh_vars.push_back(retv);
        return retv;
    }

  protected:
    AigFactory &factory;
    std::ostream &out() {
        for (int i = dbg_off; i; --i)
            OUT << ' ';
        return OUT;
    }
    SATSolver s;
    std::unique_ptr<Encoder<SATSolver>> enc;
    FreshVars fresh;
    vector<Var> free;
    vector<Var> fresh_vars;
};
