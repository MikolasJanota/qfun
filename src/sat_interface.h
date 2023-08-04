/*
 * File:  sat_interface.h
 * Author:  mikolas
 * Created on:  Tue Aug 1 10:23:36 CEST 2023
 * Copyright (C) 2023, Mikolas Janota
 */
#pragma once

/* 
 *  The QBF solver communicates with the SAT solver through the minsat
 *  interface. We have a thin wrapper around the ipasir, to use other solvers.
 *
 *  This should be more elegant but this is how it evolved historically.
 */

#if USE_IPASIR

#define SATSPC Minisat
#include "ipasir_wrap.h"
typedef SATSPC::IPASIRWrap SATSOLVER;

#else

#define SATSPC Minisat
#include "minisat_ext.h"
typedef SATSPC::MiniSatExt SATSOLVER;

#endif
