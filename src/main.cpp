/*
 * File:  main.cc
 * Author:  mikolas
 * Created on:  Mon, Jul 13, 2015 1:58:05 PM
 * Copyright (C) 2015, Mikolas Janota
 */
#include "CLI11.hpp"
#include "auxiliary.h"
#include "options.h"
#include "qfun_qcir_parser.h"
#include "qsolver.h"
#include "rareqs.h"
#include "readq.h"
#include "version.h"
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>      // for operator<<, endl, basic_ostream
#include <memory>        // for allocator, allocator_traits<>:...
#include <stdexcept>     // for invalid_argument, out_of_range
#include <string>        // for operator<<, string, char_traits
#include <unordered_map> // for operator==, _Node_const_iterator
#include <utility>       // for make_pair, pair
#include <vector>        // for vector<>::iterator, vector
#include <zlib.h>
using namespace std;

bool ends_with(const std::string &filename, const char *suf);

static void SIG_handler(int signum);
void log_header(const string &filename, const Prefix &pref, int argc,
                char const *const *argv);
int run_solver(QSolver *ps);
int run_solver(const Options &options, Rareqs *ps);
ostream &print_winning_move(ostream &o, const Move &wm);
void block_move(const Move &wm, Rareqs *ps);

extern "C" {
const char *ipasir_signature(void);
}

QfunQCIRParser *qcir_parser = nullptr;
QSolver *qps = nullptr;
Rareqs *ps = nullptr;
AigFactory *factory = nullptr;

void normalize_prefix(Prefix &p) {
    if (p.empty()) {
        p.push_back(Quantification(EXISTENTIAL, VarVector()));
        return;
    }
    size_t last = 0; // maitain the first quantifier
    for (size_t i = 1; i < p.size(); ++i) {
        auto &vs_i = p[i].second;
        if (vs_i.empty())
            continue;
        if (p[i].first == p[last].first) {
            auto &vs_last = p[last].second;
            vs_last.insert(vs_last.end(), vs_i.begin(), vs_i.end());
        } else {
            ++last;
            if (last != i)
                p[last] = p[i];
        }
    }
    size_t new_sz = last + 1;
    p.resize(new_sz);
}

int run_qcir(Options &options, [[maybe_unused]] int argc,
             [[maybe_unused]] char **argv) {
    const bool use_std = options.file_name == "-";
    gzFile in =
        use_std ? gzdopen(0, "rb") : gzopen(options.file_name.c_str(), "rb");
    if (in == nullptr) {
        cerr << "ERROR! Could not open file: " << options.file_name << endl;
        exit(EXIT_FAILURE);
    }

    StreamBuffer buf(in);
    qcir_parser = new QfunQCIRParser(buf, *factory);
    qcir_parser->parse();
    gzclose(in);
    auto &qcir_qfla = qcir_parser->formula();
    if (options.verbose >3)
      factory->print_fancy(std::cout << "input:" << qcir_qfla.pref << "(" << endl, qcir_qfla.matrix, 2) << ")" << endl;

    normalize_prefix(qcir_qfla.pref);
    // factory.print(cerr<<"read:\n", qcir_qfla.matrix)<<endl;
    // ps=new QSolver(qcir_qfla,options,factory);
    const bool rqs =
        qcir_qfla.pref.size() > 5 && var_count(qcir_qfla.pref) < 300;
    if (rqs)
        qps = new QSolver(qcir_qfla, options, *factory);
    else
        ps = Rareqs::make_solver(options, *factory, qcir_qfla.pref,
                                 qcir_qfla.matrix);
    LOG_LRN(log_header(flafile, qcir_qfla.pref, argc, argv););
    return rqs ? run_solver(qps) : run_solver(options, ps);
}

int run_cnf(const string &flafile, Options &options, [[maybe_unused]] int argc,
            [[maybe_unused]] char **argv) {
    std::unique_ptr<Reader> fr;
    gzFile ff = Z_NULL;
    if (flafile.size() == 1 && flafile[0] == '-') {
        fr.reset(new Reader(cin));
    } else {
        ff = gzopen(flafile.c_str(), "rb");
        if (ff == Z_NULL) {
            cerr << "ERROR: "
                 << "Unable to open file: " << flafile << endl;
            cerr << "ABORTING" << endl;
            exit(100);
        }
        fr.reset(new Reader(ff));
    }
    ReadQ rq(*fr, false);
    try {
        rq.read();
    } catch (ReadException &rex) {
        cerr << "ERROR: " << rex.what() << endl;
        cerr << "ABORTING" << endl;
        exit(100);
    }
    cout << "c done reading: " << read_cpu_time() << std::endl;
    if (ff != Z_NULL)
        gzclose(ff);
    if (!rq.get_header_read()) {
        cerr << "ERROR: Missing header." << endl;
        cerr << "ABORTING" << endl;
        exit(100);
    }
    LOG_LRN(log_header(flafile, rq.get_prefix(), argc, argv););

    QAigFla qf;
    AigUtil au(*factory);
    qf.pref = rq.get_prefix();
    qf.matrix = au.convert(rq.get_clauses());
    if (qf.pref.size() == 0) {
        qf.pref.push_back(make_pair(EXISTENTIAL, VarVector()));
    }
    ps = Rareqs::make_solver(options, *factory, qf.pref, qf.matrix);
    return run_solver(options, ps);
}

int main(int argc, char **argv) {
    // test_satsyn(); return 0;
    cout << "c WARNING: this still needs to be tested" << endl;
    srand(0);
    signal(SIGTERM, SIG_handler);
    signal(SIGINT, SIG_handler);
    signal(SIGABRT, SIG_handler);
#ifndef __MINGW32__
    signal(SIGHUP, SIG_handler);
    signal(SIGUSR1, SIG_handler);
#else
    cout << "c MINGW version." << endl;
#endif
#ifndef NDEBUG
    cout << "c DEBUG version." << endl;
#endif
    cout << "c qfun, v01.0, " << Version::GIT_SHA1 << ", " << Version::GIT_DATE
         << endl;
    cout << "c (C) 2017 Mikolas Janota, mikolas.janota@gmail.com" << endl;

#ifdef USE_IPASIR
    cout << "c solver (via IPASIR) " << ipasir_signature() << endl;
#elif defined(USE_MINISAT)
    cout << "c solver MINISAT" << endl;
#endif

    CLI::App app("qfun non-CNF QBF solver.");
    Options options;
    app.add_option("file_name", options.file_name,
                   "Input file name, use - (dash) or empty for stdin.")
        ->default_val("-");
    app.add_flag("-a, !--no-a", options.accum, "Accumulate strategies.")
        ->default_val(true);
    app.add_flag("-c, !--no-c", options.cyclic, "Cyclic magic function.")
        ->default_val(true);
    app.add_flag("-p, !--no-p", options.proximity, "Use proximity.")
        ->default_val(true);
    app.add_flag("-r, !--no-r", options.rndmodel, "Randomize models.")
        ->default_val(false);
    app.add_flag("-s, !--no-s", options.sample, "Initial sampling.")
        ->default_val(true);
    app.add_flag("-l, !--no-l", options.learn, "Use learning.")
        ->default_val(true);
    app.add_flag("-E, !--no-enum", options.win_mv_enum,
                 "Enumerated winning moves for top-level.")
        ->default_val(false);
    app.add_option("-S, --seed", options.seed, "Random seed.")->default_val(7);
    app.add_option("-b, --blocking", options.blocking,
                   "Clause blocking for quant levels <LEV>.")
        ->default_val(7);
    app.add_option("-i, --interval", options.interval, "Learning interval.")
        ->default_val(64);
    app.add_option("-n, --initial", options.initial,
                   "Initial refinement for quant levels <LEV>.")
        ->default_val(4);
#if EXTERNAL_SAT
    app.add_option("--external-sat", options.external_sat,
                   "External SAT solver.")
        ->default_val("");
#endif
    app.add_flag("-v", options.verbose, "Add verbosity.")->default_val(0);
    CLI11_PARSE(app, argc, argv);

    cout << "c verbosity: " << options.verbose << endl;

    factory = new AigFactory();
    const auto &flafile = options.file_name;
    int rv;
    if (ends_with(flafile, ".qcnf") || ends_with(flafile, ".qdimacs")) {
        rv = run_cnf(flafile, options, argc, argv);
    } else {
        rv = run_qcir(options, argc, argv);
    }

#ifndef NDEBUG
    if (qps)
        delete qps;
    if (ps)
        delete ps;
    if (qcir_parser)
        delete qcir_parser;
    if (factory)
        delete factory;
#endif
    return rv;
}

static void SIG_handler(int signum) {
    cout << "c received external signal " << signum << endl;
    if (ps) {
        cout << "c top level refine counter: " << ps->get_refine_counter()
             << std::endl;
        cout << "c sum refine counter: " << ps->get_sum_refine_counter()
             << std::endl;
    }
    if (qps) {
        cout << "c conflicts: " << qps->get_conflicts() << endl;
        qps->prn_refs(cout);
    }
    cout << "c Terminating ..." << endl;
    LOG_LRN(lout << "TIME: " << read_cpu_time() << endl;);
    LOG_LRN(lout << "TIME/MEM OUT" << endl;);
    exit(0);
}

void log_header(const string &filename, const Prefix &pref, int argc,
                char const *const *argv) {
    lout << "FILE:" << filename << endl;
    lout << "OPTIONS:";
    while (argc--)
        lout << " " << *(argv++);
    lout << endl;
    cout << "c qfun, v01.0, " << Version::GIT_SHA1 << ", " << Version::GIT_DATE
         << endl;
    lout << "PREFIX:" << pref << endl;
}

int run_solver(QSolver *ps) {
    const bool r = ps->solve();
    cout << "c time: " << read_cpu_time() << std::endl;
    cout << "c conflicts: " << ps->get_conflicts() << std::endl;
    cout << "s cnf " << (r ? '1' : '0') << std::endl;
    LOG_LRN(lout << "TIME: " << read_cpu_time() << endl;);
    LOG_LRN(lout << "RESULT:" << (r ? "TRUE" : "FALSE") << endl;);
    return r ? 10 : 20;
}

void log_computation(const Rareqs *ps) {
    cout << "c TIME: " << read_cpu_time() << endl;
    cout << "c top level refine counter: " << ps->get_refine_counter() << endl;
    cout << "c sum refine counter: " << ps->get_sum_refine_counter() << endl;
    cout << "c sum learning time: " << ps->get_learning_time() << endl;
}

int run_solver(const Options &options, Rareqs *ps) {
    const bool w = ps->wins();
    const bool r = (ps->quantifier_type() == EXISTENTIAL) == w;
    log_computation(ps);
    cout << "s cnf " << (r ? '1' : '0') << std::endl;
    if (w) {
        Move wm;
        while (1) {
            wm.clear();
            ps->get_move(wm);
            print_winning_move(cout, wm);
            if (!options.get_win_mv_enum())
                break;
            block_move(wm, ps);
            if (!ps->wins()) {
                cout << "c enumeration completed" << endl;
                break;
            }
            log_computation(ps);
        }
    }
    LOG_LRN(lout << "RESULT:" << (r ? "TRUE" : "FALSE") << endl;);
    return r ? 10 : 20;
}

ostream &print_winning_move(ostream &o, const Move &wm) {
    o << "v";
    for (Var v : ps->get_free()) {
        const lbool vv = eval(v, wm);
        if (vv == l_Undef)
            continue;
        const auto &var2name = qcir_parser->var2name();
        const auto i = var2name.find(v);
        o << " " << (vv == l_False ? '-' : '+');
        if (i == var2name.end())
            o << v;
        else
            o << i->second;
    }
    return o << endl;
}

void block_move(const Move &wm, Rareqs *ps) {
    LiteralVector blocking_clause;
    for (Var v : ps->get_free()) {
        const lbool vv = eval(v, wm);
        if (vv == l_Undef)
            continue;
        blocking_clause.push_back(vv == l_False ? mkLit(v) : ~mkLit(v));
    }
    auto blc = LitSet::mk(blocking_clause);
    // cerr<<"blocking_clause:"<<blc<<endl;
    ps->strengthen(blc);
}

bool ends_with(const std::string &filename, const char *suf) {
    auto l = strlen(suf);
    auto s = filename.length();
    if (static_cast<size_t>(l) > s)
        return false;
    while (l)
        if (filename[--s] != suf[--l])
            return false;
    return true;
}
