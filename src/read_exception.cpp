#include "read_exception.h"

ReadException::ReadException(const std::string &message) {
    s = new char[message.size() + 1];
    message.copy(s, message.size(), 0);
}

const char *ReadException::what() const throw() { return s; }
