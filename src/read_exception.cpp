#include "read_exception.h"

ReadException::ReadException(std::string message) : message(message) {}

const char *ReadException::what() const throw() { return message.c_str(); }
