/*
 * File:  GameCleanup.h
 * Author:  mikolas
 * Created on:  03 May 2017 13:39:57
 * Copyright (C) 2017, Mikolas Janota
 */
#ifndef GAMECLEANUP_H_5363
#define GAMECLEANUP_H_5363
#include"aig.h"
#include"qtypes.h"
#include"rareqs_types.h"
class GameCleanup {
    public:
        GameCleanup(AigFactory& factory) : factory(factory) {}
        virtual ~GameCleanup() {}
        void run(Game& g);
    private:
        AigFactory& factory;
};
#endif /* GAMECLEANUP_H_5363 */
