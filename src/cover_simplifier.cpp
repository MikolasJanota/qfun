/*
 * File:  cover_simplifier.cpp
 * Author:  mikolas
 * Created on:  12 Feb 2017 11:37:12
 * Copyright (C) 2017, Mikolas Janota
 */
#include "cover_simplifier.h"
#include "minisat_auxiliary.h"
#include <algorithm>
using SATSPC::lit_Undef;
using SATSPC::LSet;
AigLit CoverSimplier::run() {
    //    std::vector<LSet*> positive, negative;
    //    make_samples(positive, negative);
    const auto p = simplify(trues);
    const auto n = simplify(falses);
    const auto retv = (util.size(p) <= util.size(n)) ? p : factory.neg(n);
    return retv;
}

/*
void CoverSimplier::make_samples(std::vector<LSet*>&
positive,std::vector<LSet*>& negative) { for (auto core: cores) { const Lit tl =
get_vlit(target_variable,*core); if (tl == lit_Undef) continue; auto& ts =
sign(tl) ? negative : positive; ts.push_back(new LSet()); for (auto v: domain) {
            const Lit dl = get_vlit(v, *core);
            if (dl != lit_Undef) ts.back()->insert(dl);
        }
    }
}
*/

struct LSetSzCmp {
    inline bool operator()(const LSet *a, const LSet *b) const {
        return a->size() < b->size();
    }
};

// LSet* CoverSimplier::resolve(LSet& a, LSet& b, Lit pivot) {
LSet *resolve(LSet &a, LSet &b, Lit pivot) {
    bool found = false;
    auto *retv = new LSet();
    for (int i = 0; i < a.size(); ++i) {
        const Lit l = a[i];
        assert(var(l) != var(pivot) || l == pivot);
        if (l == pivot)
            found = true;
        else
            retv->insert(l);
    }
    if (!found)
        assert(0);
    found = false;
    const Lit npivot = ~pivot;
    for (int i = 0; i < b.size(); ++i) {
        const Lit l = b[i];
        assert(var(l) != var(pivot) || l == npivot);
        if (l == npivot)
            found = true;
        else
            retv->insert(l);
    }
    if (!found)
        assert(0);
    return retv;
}

bool good_ix(std::vector<LSet *> &avec, size_t ai) {
    return ai < avec.size() && avec[ai];
}

void annihilate(std::vector<LSet *> &avec, size_t ai) {
    assert(good_ix(avec, ai));
    delete avec[ai];
    avec[ai] = nullptr;
}

void subsume_chk(std::vector<LSet *> &_avec, std::vector<LSet *> &_bvec,
                 size_t _ai, size_t _bi, std::vector<LSet *> &new_terms) {
    assert(good_ix(_avec, _ai));
    assert(good_ix(_bvec, _bi));
    const bool sw = _avec[_ai]->size() > _bvec[_bi]->size();
    std::vector<LSet *> &avec = sw ? _bvec : _avec;
    std::vector<LSet *> &bvec = sw ? _avec : _bvec;
    const auto ai = sw ? _bi : _ai;
    const auto bi = sw ? _ai : _bi;

    LSet &a = *(avec[ai]);
    LSet &b = *(bvec[bi]);
    bool subsumed = true;
    bool selfsubsumed = true;
    Lit pivot = lit_Undef;
    for (int li = 0; (selfsubsumed || subsumed) && li < a.size(); ++li) {
        const Lit al = a[li];
        const Lit bl = get_vlit(var(al), b);
        if (bl == lit_Undef) {
            subsumed = selfsubsumed = false;
        } else if (bl == al) {
            // nop
        } else {
            assert(bl == ~al);
            subsumed = false;
            if (pivot != lit_Undef)
                selfsubsumed = false;
            else
                pivot = al;
        }
    }
    if (subsumed) {
        annihilate(bvec, bi);
    } else if (selfsubsumed) {
        new_terms.push_back(resolve(a, b, pivot));
        annihilate(bvec, bi);
    }
}

void shake_down(std::vector<LSet *> &vs) {
    size_t i = 0;
    while (i < vs.size()) {
        if (vs[i] == nullptr) {
            vs[i] = vs.back();
            vs.pop_back();
        } else {
            ++i;
        }
    }
}

void mv(std::vector<LSet *> &src, std::vector<LSet *> &dst) {
    for (auto *c : src)
        if (c)
            dst.push_back(c);
    src.clear();
}

AigLit CoverSimplier::simplify(std::vector<LSet *> &impls) { // TODO mem leak
    /*
    std::cerr<<"simplify impls"<<endl;
    for (auto i : impls) {
        std::cerr<<"impl:";
        for (int j = 0; j < i->size(); ++j) std::cerr<<" "<<(*i)[j];
        std::cerr<<endl;
    }
    */
    std::sort(impls.begin(), impls.end(), LSetSzCmp());
    std::vector<LSet *> tmp1, tmp2;
    std::vector<LSet *> *old = &tmp1;
    std::vector<LSet *> *active = &impls;
    std::vector<LSet *> *todo = &tmp2;

    while (!active->empty()) {
        for (size_t i = 0; i < active->size(); ++i) {
            for (size_t j = i + 1; (*active)[i] != nullptr && j < active->size();
                 ++j) {
                if ((*active)[j] == nullptr)
                    continue;
                subsume_chk(*active, *active, i, j, *todo);
            }
        }

        for (size_t i = 0; i < active->size(); ++i) {
            for (size_t j = 0; (*active)[i] != nullptr && j < old->size(); ++j) {
                if ((*old)[j] == nullptr)
                    continue;
                subsume_chk(*active, *old, i, j, *todo);
            }
        }
        shake_down(*old);
        mv(*active, *old);
        swap(active, todo);
    }
    const auto retv = disjoin(*old);
    // cleanup(*old);
    return retv;
}

AigLit CoverSimplier::disjoin(std::vector<LSet *> &impls) {
    std::vector<AigLit> tmp;
    std::vector<AigLit> tmp1;
    for (auto *c : impls) {
        assert(c);
        tmp1.clear();
        for (int i = 0; i < c->size(); ++i) {
            const Lit l = (*c)[i];
            tmp1.push_back(factory.mk_var(var(l), sign(l)));
        }
        tmp.push_back(util.and_(tmp1, false));
    }
    return util.or_(tmp, false);
}
