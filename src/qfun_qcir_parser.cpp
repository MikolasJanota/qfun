/*
 * File:  qfun_qcir_parser.cpp
 * Author:  mikolas
 * Created on:  Fri Jul 28 14:41:04 CEST 2023
 * Copyright (C) 2023, Mikolas Janota
 */

#include "qfun_qcir_parser.h"

void QfunQCIRParser::semantic_error(std::string msg) {
    std::cerr << "ERROR:" << d_ln << ": " << msg << std::endl;
    exit(100);
}
