/*
 * File:  qsolver.cpp
 * Author:  mikolas
 * Created on:  10 Jan 2017 14:42:01
 * Copyright (C) 2017, Mikolas Janota
 */
#include "qsolver.h"
#include "aig_util.h"
#include "auxiliary.h" // for OUT, LOG, LOG_LRN, __TIMECODE, uint
#include "lit_bitset.h"
#include "proximity.h"
#include "proximity_table.h"
#include <algorithm>
#include <cstdlib>
#include <iostream> // for operator<<, basic_ostream::operator<<
using SATSPC::lit_Undef;
using std::endl;
QSolver::QSolver(const QAigFla &f, const Options &_options,
                 AigFactory &_factory)
    : formula(robustify(f)), options(_options), verb(options.get_verbose()),
      factory(_factory), level_info(formula.pref), conflicts(0) {
    init();
}

QSolver::~QSolver() {
    while (!solvers.empty()) {
        delete solvers.back();
        solvers.pop_back();
    }
}

void QSolver::init() {
    AigUtil au(factory);
    const Level lev_count = level_info.qlev_count();
    const AigLit f = formula.matrix;
    // LOG_LRN(factory.print(lout<<"FLA:", f, 2, false, true) << endl;);
    assert(lev_count >= 2);
    if (options.get_proximity()) {
        Proximity prox_calc(factory);
        __TIMECODE(prox_calc.run(f););
        prox(prox_calc, level_info);
    }
    for (Level i = 0; i < lev_count; ++i)
        solvers.push_back(
            new LevelSolver(options, *this, factory, level_info, prox, i));
    for (Level i = lev_count - 2; i < lev_count; ++i)
        get_solver(i)->strenghten(f);
    if (options.get_sample() && lev_count >= 3) {
        LOG_LRN(lout << "SAMPLING" << endl;);
        const Level dummy_lev = lev_count - 1;
        const Level last_lev = lev_count - 2;
        const Level penultimate_lev = lev_count - 3;
        auto last = get_solver(last_lev);
        auto dummy = get_solver(dummy_lev);
        auto penultimate = get_solver(penultimate_lev);
        vec<Lit> assumptions;
        for (Level l = penultimate_lev; l <= last_lev; ++l)
            get_solver(l)->set_randomize(2);
        LitBitSet to_try;
        for (Var v : level_info.level_vars(last_lev)) {
            to_try.add(mkLit(v));
            to_try.add(~mkLit(v));
        }
        for (uint i = 0; i < 1024 && !to_try.empty(); ++i) {
            assumptions.clear();
            if (!penultimate->solve(assumptions))
                break;
            for (Level l = 0; l <= penultimate_lev; ++l) {
                // push opponent's moves onto last
                if (level_info.level_type(l) != level_info.level_type(last_lev))
                    update_assumptions(assumptions, l, *penultimate);
            }
            Lit al = lit_Undef;
            if (!to_try.empty()) {
                auto j = to_try.begin();
                for (int i = rand() % to_try.size(); i > 0; --i)
                    ++j;
                assert(j != to_try.end());
                al = *j;
            }
            if (al == lit_Undef) {
                if (!last->solve(assumptions))
                    break;
            } else {
                assumptions.push(al);
                if (!last->solve(assumptions)) {
                    assumptions.pop();
                    if (!last->solve(assumptions))
                        break;
                } else {
                    for (Var v : level_info.level_vars(last_lev))
                        to_try.remove(mkLit(v, last->get_val(v) == l_False));
                    assert(!to_try.get(al));
                }
            }
            for (Level l = 0; l <= last_lev; ++l) {
                // push all the moves onto dummy
                if (level_info.level_type(l) !=
                    level_info.level_type(dummy_lev))
                    update_assumptions(assumptions, l, *last);
            }
            if (dummy->solve(assumptions))
                assert(0);
            penultimate->block(dummy->core());
            penultimate->add_core(dummy->core());
            if (i && !(i % 32)) {
                penultimate->learn(last_lev);
                penultimate->cleanup_cores();
            }
        }
        for (Level l = penultimate_lev; l <= last_lev; ++l)
            get_solver(l)->set_randomize(options.get_rndmodel());
        LOG_LRN(lout << "END SAMPLING" << endl;);
    }
}

bool QSolver::solve() {
    const bool retv = _solve();
    prn_refs(std::cout);
    return retv;
}

bool QSolver::_solve() {
    Level level = 0;
    vec<Lit> assumptions;
    while (true) {
        LOG(3, OUT << "level:" << formula.pref[level].first << level
                   << std::endl;);
        LOG(3, OUT << "assumptions:" << assumptions << std::endl;);
        auto &s = *(get_solver(level));
        const bool won = s.solve(assumptions);
        if (won) {
            update_assumptions(assumptions, level, s);
            ++level;
        } else {
            ++conflicts;
            if (verb) {
                OUT << "conflicts:" << conflicts << std::endl;
            }
            if (verb && (conflicts % 100) == 0) {
                prn_refs(OUT);
            }
            if (level < 2)
                return formula.pref[level].first == UNIVERSAL;
            level = backtrack(s.core(), assumptions, level);
        }
    }
}

Level QSolver::backtrack(const SATSPC::LSet &core, vec<Lit> &assumptions,
                         Level level) {
    assert(level >= 2);
    const auto qt = formula.pref[level].first;
    Level retv = 0;
    if (0) {
        while (formula.pref[retv].first != qt)
            ++retv;
        assert(retv < 2);
        for (int i = 0; i < core.size(); ++i) {
            const auto l = core[i];
            const auto lqt = level_info.type(l);
            if (lqt != qt)
                continue;
            retv = std::max(retv, level_info.qlev(l));
        }
    } else {
        retv = level - 2;
    }

    LOG(3, OUT << "refine at " << level << " to " << retv << std::endl;);
    get_solver(retv)->refine(core, get_solver(level)->get_abstraction());
    int i = assumptions.size();
    while (i--) {
        if (level_info.qlev(assumptions[i]) < retv)
            continue;
        assumptions[i] = assumptions.last();
        assumptions.pop();
    }

    return retv;
}

void QSolver::update_assumptions(vec<Lit> &assumptions, Level level,
                                 const LevelSolver &s) {
    for (Var v : formula.pref[level].second)
        assumptions.push(mkLit(v, s.get_val(v) == l_False));
}

std::ostream &QSolver::prn_refs(std::ostream &o) {
    o << "c REFS:";
    const Level lev_count = level_info.qlev_count();
    for (Level i = 0; i < lev_count; ++i) {
        const auto c = get_solver(i)->get_frefine_counter();
        if (c)
            o << " " << formula.pref[i].first << i << ":" << c;
    }
    return o << std::endl;
}
