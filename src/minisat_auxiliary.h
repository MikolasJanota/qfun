/*
 * File:   minisat_aux.hh
 * Author: mikolas
 *
 * Created on October 21, 2010, 2:10 PM
 */
#ifndef MINISAT_AUX_HH
#define MINISAT_AUX_HH
#include "auxiliary.h"
#include "minisat/core/SolverTypes.h"
#include "sat_interface.h"
#include <ostream>
using SATSPC::l_False;
using SATSPC::l_True;
using SATSPC::l_Undef;
using SATSPC::lbool;
using SATSPC::Lit;
using SATSPC::mkLit;
using SATSPC::sign;
using SATSPC::var;
using SATSPC::Var;
using SATSPC::vec;
using std::ostream;
using std::vector;
typedef std::vector<Lit> LiteralVector;

ostream &print_model(ostream &out, const vec<lbool> &lv, const vector<Var> &vs);
ostream &print_model(ostream &out, const vec<lbool> &lv);
ostream &print_model(ostream &out, const vec<lbool> &lv, int l, int r);
ostream &print(ostream &out, const vec<Lit> &lv);
ostream &print(ostream &out, const vector<Lit> &lv);

inline ostream &operator<<(ostream &out, const Lit &l) {
    if (l == SATSPC::lit_Undef)
        return out << "lit_Undef";
    if (l == SATSPC::lit_Error)
        return out << "lit_Error";
    return out << (sign(l) ? "-" : "+") << var(l);
}

ostream &operator<<(ostream &outs, lbool lb);
ostream &operator<<(ostream &outs, const LiteralVector &ls);
ostream &operator<<(ostream &outs, const std::vector<Var> &vs);

inline lbool lbool_and(lbool a, lbool b) {
    if (a == l_False || b == l_False)
        return l_False;
    if (a == l_True && b == l_True)
        return l_True;
    return l_Undef;
}

inline lbool lbool_neg(lbool lv) {
    if (lv == l_True)
        return l_False;
    if (lv == l_False)
        return l_True;
    assert(lv == l_Undef);
    return l_Undef;
}

inline ostream &operator<<(ostream &outs, const vec<Lit> &lv) {
    return print(outs, lv);
}

inline Lit to_lit(Var v, lbool val) {
    return val == l_True ? mkLit(v) : ~mkLit(v);
}

inline Lit to_lit(const vec<lbool> &bv, Var v) {
    return (v < bv.size()) && (bv[v] == l_True) ? mkLit(v) : ~mkLit(v);
}

inline void set(Var v, lbool val, vec<lbool> &vals) {
    if (v >= vals.size())
        vals.growTo(v + 1, l_Undef);
    vals[v] = val;
}

inline lbool eval(Var v, const vec<lbool> &vals) {
    return (v < vals.size()) ? vals[v] : l_Undef;
}

inline void set(const std::vector<Var> &vars, const vec<lbool> &src_vals,
                vec<lbool> &dst_vals) {
    for (Var v : vars)
        set(v, eval(v, src_vals), dst_vals);
}

inline lbool eval(Lit l, const vec<lbool> &vals) {
    const Var v = var(l);
    if (v >= vals.size())
        return l_Undef;
    const auto vv = vals[v];
    if (vv == l_Undef)
        return l_Undef;
    return (vv == l_False) == (sign(l)) ? l_True : l_False;
}

inline void to_lits(const vec<lbool> &bv, vec<Lit> &output, int s,
                    const int e) {
    for (int index = s; index <= e; ++index) {
        if (bv[index] == l_True)
            output.push(mkLit((Var)index));
        else if (bv[index] == l_False)
            output.push(~mkLit((Var)index));
    }
}

inline void to_lits(const std::vector<Var> &vars, const vec<lbool> &bv,
                    vec<Lit> &output, bool complete) {
    for (Var v : vars) {
        lbool val = eval(v, bv);
        if (complete && val == l_Undef)
            val = l_False;
        if (val == l_True)
            output.push(mkLit(v));
        else if (val == l_False)
            output.push(~mkLit(v));
    }
}

// representative -> AND l_i
inline void encode_and_pos(SATSOLVER &solver, Lit representative,
                           const vector<Lit> &rhs) {
    for (size_t i = 0; i < rhs.size(); ++i)
        solver.addClause(~representative, rhs[i]);
}

// AND l_i -> representative
inline void encode_and_neg(SATSOLVER &solver, Lit representative,
                           const vector<Lit> &rhs) {
    vec<Lit> ls(rhs.size() + 1);
    for (size_t i = 0; i < rhs.size(); ++i)
        ls[i] = ~rhs[i];
    ls[rhs.size()] = representative;
    solver.addClause_(ls);
}

inline void encode_and(SATSOLVER &solver, Lit representative,
                       const vector<Lit> &rhs) {
    vec<Lit> ls(rhs.size() + 1);
    for (size_t i = 0; i < rhs.size(); ++i) {
        ls[i] = ~rhs[i];
        solver.addClause(~representative, rhs[i]);
    }
    ls[rhs.size()] = representative;
    solver.addClause_(ls);
}

class Lit_equal {
  public:
    inline bool operator()(const Lit &l1, const Lit &l2) const {
        return l1 == l2;
    }
};

class Lit_hash {
  public:
    inline size_t operator()(const Lit &l) const { return SATSPC::toInt(l); }
};

inline size_t literal_index(Lit l) {
    assert(var(l) > 0);
    const size_t v = (size_t)var(l);
    return sign(l) ? v << 1 : ((v << 1) + 1);
}

inline Lit index2literal(size_t l) {
    const bool positive = (l & 1);
    const Var variable = l >> 1;
    return positive ? mkLit(variable) : ~mkLit(variable);
}

/*
inline std::ostream& operator << (std::ostream& o, const SATSPC::vec<Lit>& vs) {
    for (int i = 0; i < vs.size(); ++i) o << (i ? ' ' : '[') << vs[i];
    return o << ']';
}*/

inline std::ostream &operator<<(std::ostream &o, const SATSPC::LSet &core) {
    o << '[';
    for (int i = 0; i < core.size(); ++i)
        o << (i ? " " : "") << core[i];
    return o << ']';
}

inline Lit get_vlit(SATSPC::Var v, SATSPC::LSet &core) {
    const auto pl = SATSPC::mkLit(v, false);
    const auto nl = SATSPC::mkLit(v, true);
    return core.has(pl) ? pl : (core.has(nl) ? nl : SATSPC::lit_Undef);
}

inline SATSPC::Var maxv(std::vector<SATSPC::Var> vs) {
    SATSPC::Var retv = -1;
    for (auto v : vs) {
        assert(v >= 0);
        if (v > retv)
            retv = v;
    }
    return retv;
}

namespace std {
template <> struct hash<pair<SATSPC::Var, SATSPC::Var>> {
    size_t operator()(const pair<Var, Var> &p) const {
        return p.first ^ p.second;
    }
};
};     // namespace std
#endif /* MINISAT_AUX_HH */
