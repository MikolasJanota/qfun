/*
 * File:  VarManager.h
 * Author:  mikolas
 * Created on:  Mon Jan 30 14:02:04 WET 2017
 * Copyright (C) 2017, Mikolas Janota
 */
#pragma once
#include "qtypes.h"
class VarManager {
  public:
    virtual void alloc_var(SATSPC::Var v, Level ql) = 0;
    virtual SATSPC::Var fresh_var(Level ql) = 0;
};
