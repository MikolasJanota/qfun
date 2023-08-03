/*
 * File:  satwrapper.cpp
 * Author:  mikolas
 * Created on:  Wed Apr 24 11:05:42 DST 2019
 * Copyright (C) 2019, Mikolas Janota
 */
#include "sat_wrapper.h"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>
#include <ostream>
#include <sstream>
#include <string>
using std::endl;
using std::string;
namespace Minisat {
std::mt19937 SatWrapper::rgen(0);
bool SatWrapper::solve() {
    /* static size_t stamp = 0; */
    /* stamp++; */
    const size_t stamp = std::uniform_int_distribution<size_t>(0, 10000)(rgen);
    const string input_filename =
        "/tmp/external_solver_wrapper_in" + std::to_string(stamp) + ".cnf";
    const string output_filename =
        "/tmp/external_solver_wrapper_out" + std::to_string(stamp) + ".cnf";
    const string solver_command = sat_solver_path;
    const string command =
        solver_command + " " + input_filename + " >" + output_filename;
    std::ofstream pf(input_filename);
    pf << "p cnf " << nvars << " " << cls.size() << endl;
    for (const auto p : cls) {
        for (int i = 0; i < p->size(); ++i) {
            const auto l = (*p)[i];
            pf << (Minisat::sign(l) ? "-" : "") << (1 + Minisat::var(l)) << " ";
        }
        pf << " 0" << endl;
    }
    model.clear();
    model.growTo(nvars + 1);
    const int stat_val = system(command.c_str());
    const bool ok = WIFEXITED(stat_val);
    const int ec = WEXITSTATUS(stat_val);
    if (!ok || (ec != 10 && ec != 20)) {
        std::cerr << "solver didn't run correctly  (exit code:" << ec << ")"
                  << endl;
        exit(1);
    }
    std::cerr << "solver ran correctly" << endl;
    const bool rv = ec == 10;
    if (rv) {
        string line;
        std::ifstream response_file(output_filename);
        while (getline(response_file, line)) {
            std::istringstream iss(line);
            std::vector<std::string> es(std::istream_iterator<std::string>{iss},
                                        std::istream_iterator<std::string>());
            if (es.empty())
                continue;
            if (es[0] != "v")
                continue;
            std::cerr << "model line:" << line << endl;
            for (size_t i = 1; i < es.size(); ++i) {
                long val;
                std::istringstream iss(es[i]);
                iss >> val;
                if (iss.good()) {
                    std::cerr << "error reading model line: " << line << endl;
                    exit(100);
                }
                if (val == 0)
                    break;
                const bool sign = val < 0;
                const Var v = (sign ? -val : val) - 1;
                if (v >= nVars()) {
                    std::cerr << "error reading model line: " << line << endl;
                    std::cerr << "out of range var: " << val << endl;
                    exit(100);
                }
                model[v] = sign ? Minisat::l_False : Minisat::l_True;
            }
        }
    }
    if (1) {
        std::remove(input_filename.c_str());
        std::remove(output_filename.c_str());
    }
    return rv;
}
bool SatWrapper::addClause_(const vec<Lit> &ps) {
    auto p = new vec<Lit>();
    ps.copyTo(*p);
    cls.push_back(p);
    for (int i = 0; i < p->size(); ++i) {
        const auto l = (*p)[i];
        std::cerr << (Minisat::sign(l) ? "-" : "") << Minisat::var(l) << " ";
    }
    std::cerr << " 0" << endl;
    return true;
}
} // namespace Minisat
