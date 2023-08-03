#pragma once
#include "qtypes.h"
#include <utility>                     // for pair
class LevelInfo {
  public:
    explicit LevelInfo(const Prefix &pref);
    LevelInfo(const LevelInfo &o) = delete;

    inline Level qlev(Var v) const {
        const auto vi = v_ix(v);
        assert(vi < vis.size());
        return vis[vi].second;
    }

    inline QuantifierType type(Var v) const {
        const auto vi = v_ix(v);
        assert(vi < vis.size());
        return vis[vi].first;
    }

    inline Level qlev(Lit l) const { return qlev(var(l)); }

    inline QuantifierType type(Lit l) const { return type(var(l)); }

    inline size_t qlev_count() const { return pref.size(); }

    inline Var maxv() const { return mxv; }

    inline const VarVector &level_vars(Level lev) const {
        return pref[lev_ix(lev)].second;
    }

    inline QuantifierType level_type(Level lev) const {
        return pref[lev_ix(lev)].first;
    }

    inline Level max_qlev() const { return ((Level)qlev_count()) - 1; }

    void add_var(Var v, Level l);

    inline const Prefix &prefix() const { return pref; }

    bool depends_on(Var target_variable, Var src_variable) const {
        const Level tl = qlev(target_variable);
        const Level sl = qlev(src_variable);
#if 1
        return sl < tl;
#else
        return sl < tl || (tl == sl && src_variable < target_variable);
#endif
    }

  private:
    Prefix pref;
    Var mxv;
    std::vector<std::pair<QuantifierType, Level>> vis;

    size_t v_ix(Var v) const {
        assert(v >= 0);
        const auto ix = (size_t)v;
        return ix;
    }

    size_t lev_ix(Level ql) const {
        assert(ql >= 0);
        const auto ix = (size_t)ql;
        assert(ix < pref.size());
        return ix;
    }
};

inline ostream &operator<<(ostream &outs, const LevelInfo &levs) {
    for (size_t ql = 0; ql < levs.qlev_count(); ++ql) {
        outs << "[" << levs.level_type(ql);
        for (auto v : levs.level_vars(ql))
            outs << " " << v;
        outs << "]";
    }
    return outs;
}
