/*
 * File:  level_solver.h
 * Author:  mikolas
 * Created on:  10 Jan 2017 19:33:48
 * Copyright (C) 2017, Mikolas Janota
 */
#pragma once
#include "aig.h"
#include "aig_util.h"
#include "core.h"
#include "encoder.h"
#include "level_info.h"
#include "litset.h"
#include "options.h"
#include "proximity_table.h"
#include "sat_interface.h"
#include "var_manager.h"
#include "varset.h"
class LevelSolver {
  public:
    LevelSolver(const Options &options, VarManager &vm, AigFactory &factory,
                const LevelInfo &li, const ProximityTable &prox, Level qlevel);
    virtual ~LevelSolver();
    void add_clause(LitSet cl);
    void strenghten(const AigLit f);
    void refine(const SATSPC::LSet &core, AigLit fla);
    bool solve(const vec<Lit> &assumptions);
    lbool get_val(Var v) const;
    const SATSPC::LSet &core() const { return sat->get_conflict(); }
    AigLit get_abstraction() const { return abstraction; }
    size_t get_frefine_counter() const { return frefine_counter; }
    QuantifierType get_qt() const { return qtype; }
    Var fresh_var(Level ql);
    void alloc_var(Var v) { return encoder.alloc_var(v); }
    void learn(Level strat_level);
    void add_core(const SATSPC::LSet &core);
    void block(const SATSPC::LSet &core);
    int set_randomize(int v) { return randomize = v; }
    void cleanup_cores() { cores.clear(); }

  private:
    const Options &options;
    const int verb;
    int randomize;
    VarManager &var_mng;
    LevelInfo level_info;
    AigFactory &factory;
    const ProximityTable &prox;
    SATSOLVER *sat;
    Encoder<SATSOLVER> encoder;
    AigUtil util;
    const Level qlevel;
    const QuantifierType qtype;
    const CNF cnf;
    size_t refine_counter;
    size_t frefine_counter;
    AigLit abstraction;
    vector<Core> cores;
    std::unordered_map<Var, AigLit> strategies;
    void make_blocking_clause(const SATSPC::LSet &core,
                              vec<Lit> &blocking_clause);
    void refine_fancy(const SATSPC::LSet &core, Level strat_level, AigLit fla);
    AigLit get_input(SATSPC::LSet &core, const VarSet &domain);
    void refine_fancy(const SATSPC::LSet &core, Level strat_level);
    // void get_domain(Var v, VarSet& domain, Level strat_level);
    void get_implicants(Var v, Level strat_level, VarSet &domain,
                        vector<SATSPC::LSet *> &trues,
                        vector<SATSPC::LSet *> &falses);
    AigLit subs(const std::unordered_map<Var, AigLit> &strats, AigLit fla);
    bool get_subs(std::unordered_map<size_t, AigLit> &node_cache,
                  std::unordered_map<Var, AigLit> &subs, const AigLit &t,
                  AigLit &res);
    std::ostream &prn_sample(std::ostream &o, const Core &core,
                             Level strat_level, size_t limit = 0);
    ostream &prn_sample(ostream &o, Core &core, const VarSet &domain, Var v,
                        size_t limit = 0);
    bool magic(size_t refine_counter, Level strat_level);
    bool is_good_strategy(const Core &core, Var v, const AigLit &s,
                          vector<Core> &lacks_response);
    bool update_strategy(const AigLit &s, Var v, vector<Core> cores,
                         AigLit &updated);
    std::ostream &prn_strat(std::ostream &o, Var v, AigLit s,
                            const char *const comment = "");
    bool depends_on(Var target_variable, Var src_variable) const;
    void diff(Var target_variable, const Core &a, const Core &b,
              std::unordered_set<Var> &vs) const;
};
