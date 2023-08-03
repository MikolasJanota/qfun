/*
 * File:  aig.h
 * Author:  mikolas
 * Created on:  15 Jan 2017 14:51:23
 * Copyright (C) 2017, Mikolas Janota
 */
#ifndef AIG_H_27844
#define AIG_H_27844
#include <ostream>
#ifdef NODESHASH
#include <unordered_map>
#else
#include <map>
#endif
#include "auxiliary.h"
#include <vector>

class AigLit {
  public:
    enum const_type { FALSE, TRUE };
    enum lit_type { CONST, VAR, NODE };
    AigLit() {}
    inline static AigLit mk_node(size_t ptr, bool sign) {
        return AigLit(NODE, sign, ptr);
    }
    inline static AigLit mk_var(int v, bool sign) {
        assert(v >= 0);
        return AigLit(VAR, sign, (size_t)v);
    }
    lit_type type() const { return _type; }
    int var() const {
        assert(type() == VAR);
        return data;
    }
    size_t ptr() const {
        assert(type() == NODE);
        return data;
    };
    size_t sign() const {
        assert(type() == NODE || type() == VAR);
        return _sign;
    }
    inline bool is_var() const { return type() == VAR; }
    inline bool is_node() const { return type() == NODE; }
    inline bool is_const() const { return type() == CONST; }
    inline bool is_true() const { return type() == CONST && data; }
    inline bool is_false() const { return type() == CONST && !data; }
    inline static AigLit mk_true() { return AigLit(CONST, false, 1); }
    inline static AigLit mk_false() { return AigLit(CONST, false, 0); }
    inline AigLit neg() const {
        switch (_type) {
        case VAR: return AigLit(VAR, !sign(), data);
        case NODE: return AigLit(NODE, !sign(), data);
        case CONST: return AigLit(CONST, false, !data);
        }
        assert(0);
        return AigLit();
    }
    inline size_t hash() const { return (size_t)_sign ^ data ^ (size_t)_type; }
    inline bool equals(const AigLit &o) const {
        return _type == o._type && data == o.data && _sign == o._sign;
    }
    inline static int cmp(const AigLit &a, const AigLit &b) {
        if (a._type < b._type)
            return -1;
        if (a._type > b._type)
            return +1;
        if (a.data < b.data)
            return -1;
        if (a.data > b.data)
            return +1;
        if (a._sign == b._sign)
            return 0;
        return !a._sign ? -1 : +1;
    }
    inline static bool is_neg(const AigLit &a, const AigLit &b) {
        if (a.type() != b.type())
            return false;
        if (a.is_var())
            return a.var() == b.var() && a.sign() != b.sign();
        if (a.is_node())
            return a.ptr() == b.ptr() && a.sign() != b.sign();
        if (a.is_true())
            return b.is_false();
        if (a.is_false())
            return b.is_true();
        assert(0);
        return false;
    }

    inline static size_t to_int(const AigLit &l) {
        return l.data << 3 | (l._sign ? 1 : 0) | (to_int(l._type) << 1);
    }

    inline static AigLit from_int(size_t i) {
        AigLit r;
        r._sign = (i & 1) == 1;
        i >>= 1;
        r._type = type_from_int(i & 3);
        i >>= 2;
        r.data = i;
        return r;
    }

  private:
    lit_type _type;
    bool _sign;
    size_t data;
    AigLit(lit_type t, bool s, size_t data) : _type(t), _sign(s), data(data) {}

    inline static lit_type type_from_int(size_t i) {
        switch (i) {
        case 0: return CONST;
        case 1: return VAR;
        case 2: return NODE;
        }
        assert(0);
        return CONST;
    }
    inline static size_t to_int(lit_type t) {
        switch (t) {
        case CONST: return 0;
        case VAR: return 1;
        case NODE: return 2;
        }
        assert(0);
        return -1;
    }
};

inline bool operator<(const AigLit &n1, const AigLit &n2) {
    return AigLit::cmp(n1, n2) < 0;
}

class AigNode {
  public:
    AigNode() {}
    static inline AigNode mk(const AigLit &s, AigLit &t) {
        return s < t ? AigNode(s, t) : AigNode(t, s);
    }

    inline const AigLit &a() const { return _a; }
    inline const AigLit &b() const { return _b; }
    inline size_t hash() const { return a().hash() ^ b().hash(); }
    inline bool equals(const AigNode &o) const {
        return _a.equals(o._a) && _b.equals(o._b);
    }
    inline bool less(const AigNode &o) const {
        const auto ca = AigLit::cmp(_a, o._a);
        return ca < 0 || (ca == 0 && _b < o._b);
    }

  private:
    AigLit _a, _b;
    inline AigNode(const AigLit &s, const AigLit &t) : _a(s), _b(t) {}
};

struct AigNode_hash {
    inline size_t operator()(const AigNode &n) const { return n.hash(); }
};

struct AigNode_equal {
    inline bool operator()(const AigNode &n1, const AigNode &n2) const {
        return n1.equals(n2);
    }
};

namespace std {
template <> class hash<AigLit> {
  public:
    inline size_t operator()(const AigLit &n) const { return n.hash(); }
};

template <> class equal_to<AigLit> {
  public:
    inline bool operator()(const AigLit &n1, const AigLit &n2) const {
        return n1.equals(n2);
    }
};

template <> class less<AigLit> {
  public:
    inline bool operator()(const AigLit &n1, const AigLit &n2) const {
        return n1 < n2;
    }
};

template <> class less<AigNode> {
  public:
    inline bool operator()(const AigNode &n1, const AigNode &n2) const {
        return n1.less(n2);
    }
};

}; // namespace std

class AigFactory {
  public:
    AigFactory();
    virtual ~AigFactory();

  public:
    inline AigLit neg(const AigLit &a) { return a.neg(); }
    inline AigLit _mk_and(AigLit a, AigLit b);
    inline AigLit mk_and(AigLit a, AigLit b);
    inline AigLit mk_and(AigLit a, AigLit b, AigLit c) {
        return mk_and(a, mk_and(b, c));
    }
    AigLit mk_or(AigLit a, AigLit b) { return neg(mk_and(neg(a), neg(b))); }
    AigLit mk_true() { return AigLit::mk_true(); }
    AigLit mk_false() { return AigLit::mk_false(); }
    AigLit mk_var(int v, bool sign) { return AigLit::mk_var(v, sign); };
    bool is_true(const AigLit &l) const { return l.is_true(); }
    bool is_false(const AigLit &l) const { return l.is_false(); }
    std::ostream &print(std::ostream &o, AigLit l, size_t off = 0);
    std::ostream &print_fancy(std::ostream &o, AigLit l, size_t off = 0) {
        return print(o, l, off, false, true);
    }
    std::ostream &print(std::ostream &o, AigLit l, size_t off, bool labels,
                        bool pol);
    AigNode get_node(size_t ptr) { return get_node_(ptr); }
    AigNode get_node(const AigLit &l) {
        assert(l.is_node());
        return get_node_(l.ptr());
    }
    size_t node_count() const { return nodes.size(); }

  protected:
    // std::unordered_map<AigNode,size_t,AigNode_hash,AigNode_equal> nodes_map;
    std::map<AigNode, size_t> nodes_map;
    std::vector<AigNode> nodes;
    size_t mk_node(AigLit &a, AigLit &b);
    AigNode &get_node_(size_t ptr) {
        assert(ptr < nodes.size());
        return nodes[ptr];
    }
    inline bool simp_nodes(AigLit a, AigLit b, AigLit &res);
    inline bool simp_unit(AigLit a, AigLit b, AigLit &res);
    void flatten(AigLit l, std::vector<AigLit> &res);
    void flattened_nodes(AigLit l, std::vector<AigLit> &res);
};

// # define CHK_SAME(aa, ab, ba, bb)/    do {if ((aa).equals(ba)) { found_same =
// true; same = aa; aother = ab; bother = bb; }} while (0)

inline bool chk_same(const AigLit aa, const AigLit ab, const AigLit ba,
                     const AigLit bb, AigLit &same, AigLit &aother,
                     AigLit &bother) {
    if (!(aa).equals(ba))
        return false;
    same = aa;
    aother = ab;
    bother = bb;
    return true;
}

// inline bool chk_neq(const AigLit aa, const AigLit ab, const AigLit ba, const
// AigLit bb, AigLit& same, AigLit& aother, AigLit& bother) {
//    if (!AigLit::is_neg(aa,ba)) return false;
//    same = aa; aother = ab; bother = bb;
//    return true;
//}

AigLit AigFactory::mk_and(AigLit a, AigLit b) {
    const auto retv = _mk_and(a, b);
#if 0
    print(std::cerr << "a:", a, 2) << std::endl;
    print(std::cerr << "b:", b, 2) << std::endl;
    print(std::cerr << "r:", retv, 2) << std::endl;
#endif
    return retv;
}

bool AigFactory::simp_unit(AigLit a, AigLit conj, AigLit &res) {
    if (a.is_node() && conj.is_var())
        std::swap(a, conj);
    else if (!a.is_var() || !conj.is_node())
        return false;
    assert(a.is_var() && conj.is_node());
    const auto &n = get_node_(conj.ptr());
    if (AigLit::is_neg(a, n.a()) || AigLit::is_neg(a, n.b())) {
        res = conj.sign() ? a : mk_false();
        return true;
    }
    if (a.equals(n.a())) {
        res = conj.sign() ? mk_and(a, neg(n.b())) : conj;
        return true;
    }
    if (a.equals(n.b())) {
        res = conj.sign() ? mk_and(a, neg(n.a())) : conj;
        return true;
    }
    return false;
}

bool AigFactory::simp_nodes(AigLit a, AigLit b, AigLit &res) {
    if (!a.is_node() || !b.is_node())
        return false;
    if (a.sign() && !b.sign())
        std::swap(a, b);
    const auto &na = get_node_(a.ptr());
    const auto &nb = get_node_(b.ptr());
    const auto asg = a.sign();
    const auto bsg = b.sign();
    assert(!asg || bsg);
    AigLit same, aother, bother;
    if (chk_same(na.a(), na.b(), nb.a(), nb.b(), same, aother, bother) ||
        chk_same(na.a(), na.b(), nb.b(), nb.a(), same, aother, bother) ||
        chk_same(na.b(), na.a(), nb.a(), nb.b(), same, aother, bother) ||
        chk_same(na.b(), na.a(), nb.b(), nb.a(), same, aother, bother)) {
#if 0
        print(std::cerr << "same:", same) << std::endl;
        print(std::cerr << "aother:", aother) << std::endl;
        print(std::cerr << "bother:", bother) << std::endl;
#endif
        if (asg) {
            assert(bsg);
            res = neg(mk_and(same, mk_or(aother, bother)));
        } else if (bsg) {
            assert(!asg);
            res = mk_and(neg(bother), a);
        } else {
            assert(!asg && !bsg);
            res = mk_and(same, aother, bother);
        }
        return true;
    }
    if ((!asg || !bsg) &&
        (AigLit::is_neg(na.a(), nb.a()) || AigLit::is_neg(na.a(), nb.b()) ||
         AigLit::is_neg(na.b(), nb.a()) || AigLit::is_neg(na.b(), nb.b()))) {
        if (bsg) {
            assert(!asg);
            res = a;
        } else {
            assert(!asg && !bsg);
            res = mk_false();
        }
        return true;
    }
    return false;
}

AigLit AigFactory::_mk_and(AigLit a, AigLit b) {
    if (is_true(a))
        return b;
    if (is_true(b))
        return a;
    if (is_false(a) || is_false(b))
        return mk_false();
    if (a.equals(b))
        return a;
    if (AigLit::is_neg(a, b))
        return mk_false();
#if 1
    AigLit tmp;
    if (simp_nodes(a, b, tmp))
        return tmp;
    if (simp_unit(a, b, tmp))
        return tmp;
#endif
    const auto id = mk_node(a, b);
    const auto retv = AigLit::mk_node(id, false);
    return retv;
}

inline static AigLit from_int(size_t i, AigFactory &f) {
    const auto r = AigLit::from_int(i);
    f.print(std::cerr << "from_int:", r) << std::endl << i << std::endl;
    return r;
}

inline static size_t to_int(const AigLit &l, AigFactory &f) {
    const auto r = AigLit::to_int(l);
    f.print(std::cerr << "to_int:", l) << std::endl << r << std::endl;
    return r;
}
#endif /* AIG_H_27844 */
