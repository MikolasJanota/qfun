/*
 * File:   reader.cc
 * Author: mikolas
 *
 * Created on January 12, 2011, 4:19 PM
 */

#include "reader.h"
#include "stream_buffer.h"
#include <utility>
Reader::Reader(gzFile &zf) : lnn(1), buf(zf) { update_cur(); }

Reader::~Reader() {}

void Reader::skip_line() {
    auto &me = *this;
    while (true) {
        switch (*me) {
        case EOF: return;
        case '\n': ++me; return;
        default: ++me;
        }
    }
}

void Reader::update_cur() {
    cur = *buf;
    ++buf;
    switch (cur) {
    case EOF: return;
    case '\n': ++lnn; return;
    case '\r':
        ++lnn;
        cur = '\n';
        if (*buf == '\n')
            ++buf;
        break;
    default:;
    }
}

void Reader::skip_whitespace() {
    while (((**this) >= 9 && (**this) <= 13) || (**this) == 32)
        ++(*this);
}
