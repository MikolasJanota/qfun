/*
 * File:  sat_varmanager.h
 * Author:  mikolas
 * Created on:  16 Mar 2017 17:41:52
 * Copyright (C) 2017, Mikolas Janota
 */
#pragma once
#include "sat_interface.h"
#include "var_manager.h"
class SatVarManager : public VarManager {
  public:
    SatVarManager(SATSOLVER &s) : s(s) {}

  private:
    SATSOLVER &s;
    virtual void alloc_var(Var v, Level) { s.new_variables(v); }
    virtual Var fresh_var(Level) { return s.newVar(); }
};
