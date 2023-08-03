#include "varset.h"
#include "auxiliary.h"
using std::ostream;

VarSet::VarSet() {}

void VarSet::add_all(const VarSet &o) {
    s.reserve(o.s.size());
    vs.reserve(o.vs.size());
    poss.reserve(o.poss.size());
    for (Var v : o)
        add(v);
}

void VarSet::add_all(const std::vector<Var> &variables) {
    for (Var v : variables)
        add(v);
}

ostream &VarSet::print(ostream &out) const {
    bool f = true;
    out << "[";
    for (Var v : *this) {
        out << (f ? "" : " ") << v;
        f = false;
    }
    return out << ']';
}

ostream &operator<<(ostream &outs, const VarSet &vs) { return vs.print(outs); }
