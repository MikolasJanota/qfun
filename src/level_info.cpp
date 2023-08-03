#include "level_info.h"
LevelInfo::LevelInfo(const Prefix &_pref) {
    mxv = -1;
    pref.resize(_pref.size());
    for (size_t level = 0; level < _pref.size(); ++level) {
        pref[level].first = _pref[level].first;
        const auto &vars = _pref[level].second;
        for (Var v : vars)
            add_var(v, (Level)level);
    }
}

/*
void LevelInfo::add_var(Var v, Level ql) {
  const auto lix = lev_ix(ql);
  const auto vix = v_ix(v);
  if (v>mxv) {
    mxv = v;
    vis.resize(vix+1);
  }
  vis[vix].first=level_type(ql);
  vis[vix].second=ql;
  const auto& old_vars = pref[lix].second;
  const auto old_sz = old_vars.size();
  vector<Var> vs(old_sz+1);
  for (size_t i = 0; i < old_sz; ++i) vs[i] = old_vars[i];
  vs[old_sz] = v;
  pref[lix].second = VarVector(vs);
}
*/

void LevelInfo::add_var(Var v, Level ql) {
    const auto lix = lev_ix(ql);
    const auto vix = v_ix(v);
    if (v > mxv) {
        mxv = v;
        vis.resize(vix + 1);
    }
    vis[vix].first = level_type(ql);
    vis[vix].second = ql;
    pref[lix].second.push_back(v);
}
