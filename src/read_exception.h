/*
 * File:  ReadException.hh
 * Author:  mikolas
 * Created on:  Fri Sep 2 19:43:25 GMTDT 2011
 * Copyright (C) 2011, Mikolas Janota
 */
#pragma once
#include <exception>
#include <string>
using std::exception;
class ReadException : public exception {
  public:
    ReadException(std::string message);
    const char *what() const throw();

  private:
    std::string message;
};
