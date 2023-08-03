#include "minisat_auxiliary.h"

ostream &print(ostream &out, const vec<Lit> &lv) {
    for (int i = 0; i < lv.size(); ++i)
        out << lv[i] << " ";
    return out;
}

ostream &print(ostream &out, const vector<Lit> &lv) {
    for (size_t i = 0; i < lv.size(); ++i)
        out << lv[i] << " ";
    return out;
}

ostream &operator<<(ostream &outs, const std::vector<Var> &vs) {
    for (size_t i = 0; i < vs.size(); ++i) {
        if (i)
            outs << ' ';
        outs << vs[i];
    }
    return outs;
}

ostream &operator<<(ostream &outs, const LiteralVector &ls) {
    for (size_t i = 0; i < ls.size(); ++i) {
        if (i)
            outs << ' ';
        outs << ls[i];
    }
    return outs;
}

ostream &print_model(ostream &out, const vec<lbool> &lv,
                     const vector<Var> &vs) {
    bool fst = true;
    for (Var v : vs) {
        if (fst)
            fst = false;
        else
            out << " ";
        const lbool val = lv[v];
        out << (val == l_Undef ? "?" : (val == l_True ? "+" : "-")) << v;
    }
    return out;
}

ostream &print_model(ostream &out, const vec<lbool> &lv) {
    return print_model(out, lv, 1, lv.size() - 1);
}

ostream &print_model(ostream &out, const vec<lbool> &lv, int l, int r) {
    for (int i = l; i <= r; ++i) {
        int v = 0;
        if (i > l)
            out << " ";
        if (lv[i] == l_True)
            v = i;
        else if (lv[i] == l_False)
            v = -i;
        else if (lv[i] == l_Undef)
            v = 0;
        else
            assert(false);
        out << v;
    }
    return out;
}

ostream &operator<<(ostream &outs, lbool lb) {
    if (lb == l_Undef)
        return outs << "l_Undef";
    if (lb == l_True)
        return outs << "l_True";
    if (lb == l_False)
        return outs << "l_False";
    outs << "ERROR lbool";
    exit(100);
    return outs;
}
