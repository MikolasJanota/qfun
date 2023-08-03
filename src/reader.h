/*
 * File:   Reader.hh
 * Author: mikolas
 *
 * Created on January 12, 2011, 4:19 PM
 */
#pragma once
#include <cstdio>
#include <iostream>
#include <zlib.h>
using std::istream;
class StreamBuffer;
class Reader {
  public:
    Reader(gzFile &zf);
    Reader(StreamBuffer &zipStream);
    Reader(istream &stream);
    Reader(const Reader &orig);
    virtual ~Reader();
    int operator*();
    void operator++();
    void skip_whitespace();
    inline size_t get_line_number();

  private:
    size_t lnn;
    StreamBuffer *zip;
    istream *s;
    int c;
};

inline size_t Reader::get_line_number() { return lnn; }

template <class B> static inline bool isEof(B &in) { return *in == EOF; }
template <class B> static void skipLine(B &in) {
    for (;;) {
        if (isEof(in))
            return;
        if (*in == '\n') {
            ++in;
            return;
        }
        ++in;
    }
}
