/*
 * File:   reader.cc
 * Author: mikolas
 *
 * Created on January 12, 2011, 4:19 PM
 */

#include "reader.h"
#include "stream_buffer.h"
#include <utility>
Reader::Reader(gzFile &zf) : lnn(0), zip(new StreamBuffer(zf)), s(nullptr) {}
Reader::Reader(StreamBuffer &zipStream) : lnn(0), zip(&zipStream), s(nullptr) {}
Reader::Reader(istream &stream) : lnn(0), zip(nullptr), s(&stream) {
    c = s->get();
}
Reader::Reader(const Reader &orig)
    : lnn(orig.lnn), zip(orig.zip), s(orig.s), c(orig.c) {}

Reader::~Reader() {}

int Reader::operator*() {
    const int r = s == nullptr ? **zip : c;
    return r;
}

void Reader::operator++() {
    if (s == nullptr) {
        ++(*zip);
        if ((**zip) == '\n')
            ++lnn;
    } else {
        if (s->eof())
            c = EOF;
        else
            c = s->get();
        if (c == '\n')
            ++lnn;
    }
}
void Reader::skip_whitespace() {
    while (((**this) >= 9 && (**this) <= 13) || (**this) == 32)
        ++(*this);
}
