/*
 * File:  rareqs_types.h
 * Author:  mikolas
 * Created on:  Tue, May 9, 2017 10:20:33
 * Copyright (C) 2017, Mikolas Janota
 */
#ifndef RAREQS_TYPES_H_12506
#define RAREQS_TYPES_H_12506
#include "aig.h"
#include "qtypes.h"
typedef SATSPC::vec<SATSPC::lbool> Move;
struct Game { Prefix p; AigLit m; };
struct MultiGame {
    AigLit prop;
    vector<LitSet> clauses;
    vector<Game> gs;
};
#endif /* RAREQS_TYPES_H_12506 */
