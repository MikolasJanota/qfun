/*
 * File:   auxiliary.hh
 * Author: mikolas
 *
 * Created on October 12, 2011
 */
#pragma once
#include <iostream>
#include <map>
#include <sys/time.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#ifndef __MINGW32__
#include <sys/resource.h>
#endif
#include <cassert>
#include <iomanip>

#define OUT std::cout

#define LOG(lev, code)                                                         \
    do {                                                                       \
        if (verb >= lev) {                                                     \
            code                                                               \
        }                                                                      \
    } while (0)

#ifdef LOGLRN
#define LOG_LRN(code)                                                          \
    do {                                                                       \
        code                                                                   \
    } while (0)
#else
#define LOG_LRN(code)
#endif

#define lout (std::cerr)

#ifdef __TIMING__
#define __TIMECODE(code)                                                       \
    const double _tm1245 = read_cpu_time();                                    \
    std::cerr << "Starting timer: " << __FILE__ << ":" << __LINE__ << " "      \
              << std::endl;                                                    \
    code std::cerr << "time: " << __FILE__ << ":" << __LINE__ << " "           \
                   << std::fixed << std::setprecision(2)                       \
                   << (read_cpu_time() - _tm1245) << std::endl;
#else
#define __TIMECODE(code) code
#endif

#define __PL (std::cerr << __FILE__ << ":" << __LINE__ << std::endl).flush();
#define DPRN(msg)                                                              \
    (std::cerr << __FILE__ << ":" << __LINE__ << ": " << msg << std::endl)     \
        .flush();

#ifdef __MINGW32__
static inline double read_cpu_time() { return 0; }
#else
static inline double read_cpu_time() {
    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    return (double)ru.ru_utime.tv_sec + (double)ru.ru_utime.tv_usec / 1000000;
}
#endif

#define VERIFY(c)                                                              \
    do {                                                                       \
        if (!(c))                                                              \
            assert(0);                                                         \
    } while (0)

typedef unsigned int uint;

namespace std {
template <> struct hash<std::pair<size_t, size_t>> {
    inline size_t operator()(const std::pair<size_t, size_t> &p) const {
        return p.first ^ p.second;
    }
};
} // namespace std

template <class V> std::vector<V> *mk_vec(const V &val) {
    std::vector<V> *const vs = new std::vector<V>(1);
    (*vs)[0] = val;
    return vs;
}

template <class K, class V>
bool contains(const std::unordered_map<K, V> &es, const K &e) {
    return es.find(e) != es.end();
}

template <class K, class V>
V get(const std::unordered_map<K, V> &es, const K &e) {
    const auto j = es.find(e);
    assert(j != es.end());
    return j->second;
}

inline bool contains(const std::vector<bool> &es, int e) {
    assert(e >= 0);
    const auto ix = (size_t)e;
    return ix < es.size() ? es[ix] : false;
}

template <class K, class V, class Cmp, class Alloc>
inline void insert_chk(std::map<K, V, Cmp, Alloc> &m, const K &k, const V &v) {
    assert(m.find(k) == m.end());
    m[k] = v;
}

template <class K, class V, class Hash, class Pred, class Alloc>
inline void insert_chk(std::unordered_map<K, V, Hash, Pred, Alloc> &m,
                       const K &k, const V &v) {
    assert(m.find(k) == m.end());
    m[k] = v;
}

template <class K, class V, class Cmp, class Alloc>
inline void insert_chk(std::map<K, V, Cmp, Alloc> &m,
                       typename std::map<K, V, Cmp, Alloc>::const_iterator hint,
                       const K &k, const V &v) {
    assert(m.find(k) == m.end());
    m.insert(hint, std::make_pair(k, v));
}

template <class V, class Hash, class Pred, class Alloc>
inline void insert_chk(
    std::unordered_set<V, Hash, Pred, Alloc> &m,
    typename std::unordered_map<V, Hash, Pred, Alloc>::const_iterator hint,
    const V &v) {
    assert(m.find(v) == m.end());
    m.insert(hint, v);
}

template <class K, class V, class Hash, class Pred, class Alloc>
inline void insert_chk(
    std::unordered_map<K, V, Hash, Pred, Alloc> &m,
    typename std::unordered_map<K, V, Hash, Pred, Alloc>::const_iterator hint,
    const K &k, const V &v) {
    assert(m.find(k) == m.end());
    m.insert(hint, std::make_pair(k, v));
}

template <class K>
inline bool insert_chk(std::unordered_set<K> &es, const K &e) {
    const auto i = es.insert(e);
    const bool a = i.second;
    assert(a);
    return a;
}

inline bool erase(std::vector<bool> &es, int e) {
    assert(e >= 0);
    const auto ix = (size_t)e;
    if (ix >= es.size())
        return false;
    const bool rv = es[ix];
    es[ix] = false;
    return rv;
}

inline bool insert(std::vector<bool> &es, int e) {
    assert(e >= 0);
    const auto ix = (size_t)e;
    if (ix >= es.size())
        es.resize(ix + 1, false);
    const bool rv = es[ix];
    es[ix] = e;
    return rv;
}

template <class V> bool mark(std::unordered_set<V> &es, const V &e) {
    const auto i = es.find(e);
    if (i != es.end())
        return false;
    es.insert(i, e);
    return true;
}

template <class K> bool contains(const std::unordered_set<K> &es, const K &e) {
    return es.find(e) != es.end();
}

template <class T> bool rm(std::vector<T> &vs, const T e) {
    const auto sz = vs.size();
    bool found = false;
    for (size_t i = 0; i < sz; ++i) {
        if (vs[i] == e) {
            vs[i] = vs[sz - 1];
            found = true;
            break;
        }
    }
    if (found)
        vs.pop_back();
    assert(found);
    return found;
}

inline std::ostream &operator<<(std::ostream &outs,
                                const std::vector<bool> &ns) {
    outs << '<';
    bool f = true;
    for (size_t i = 0; i < ns.size(); ++i) {
        if (!ns[i])
            continue;
        if (!f)
            outs << ' ';
        f = false;
        outs << i;
    }
    return outs << '>';
}

inline std::ostream &operator<<(std::ostream &outs,
                                const std::vector<size_t> &ns) {
    outs << '<';
    for (size_t i = 0; i < ns.size(); ++i) {
        if (i)
            outs << ' ';
        outs << ns[i];
    }
    return outs << '>';
}

template <class K, class V>
inline void cleanup_vals(std::unordered_map<K, V *> &m) {
    for (auto p : m)
        delete p.second;
}

template <class T> inline void cleanup(std::vector<T *> &vs) {
    for (auto p : vs)
        delete p;
    vs.clear();
}

template <class T> inline void pop_chk(std::vector<T> &stack, const T &fsl) {
    VERIFY(stack.back() == fsl);
    stack.pop_back();
}
