/*
 * File:  sat_interface.h
 * Author:  mikolas
 * Created on:  Tue Aug 1 10:23:36 CEST 2023
 * Copyright (C) 2023, Mikolas Janota
 */
#pragma once

/* To communicate with a SAT solver, literals are represented as a type Lit, and
 * clauses by SATCLS. The functions SATCLS_PUSH(cls,lit) is used to push
 * literals onto clauses and SATCLS_CAPACITY(cls,cap) is used to set capacity of
 * clauses.
 *
 * This should be more elegant but this is how it evolved historically.
 */

#if USE_IPASIR

#define SATSPC Minisat
#include "ipasir_wrap.h"
typedef SATSPC::IPASIRWrap SATSOLVER;
typedef SATSPC::vec<SATSPC::Lit> SATCLS;
inline void SATCLS_PUSH(SATCLS &c, const SATSPC::Lit &l) { c.push(l); }
inline void SATCLS_CAPACITY(SATCLS &c, size_t cap) { c.capacity(cap); }

#else

#define SATSPC Minisat
#include "minisat_ext.h"

typedef SATSPC::MiniSatExt SATSOLVER;
typedef SATSPC::vec<SATSPC::Lit> SATCLS;
inline void SATCLS_PUSH(SATCLS &c, const SATSPC::Lit &l) { c.push(l); }
inline void SATCLS_CAPACITY(SATCLS &c, size_t cap) { c.capacity(cap); }

#endif
