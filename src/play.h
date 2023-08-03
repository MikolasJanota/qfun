/*
 * File:  Play.h
 * Author:  mikolas
 * Created on:  15 Apr 2017 16:20:36
 * Copyright (C) 2017, Mikolas Janota
 */
#pragma once
#include "core.h"
#include "minisat_auxiliary.h"
struct Play {
    Play(Core me, Core opp) : me(me), opp(opp) {}
    Play() {}
    Play(const Play &o) : me(o.me), opp(o.opp) {}
    Core me, opp;
};

inline std::ostream &operator<<(std::ostream &o, const Play &p) {
    return o << p.me << " | " << p.opp;
}
