/*
 * File:   Reader.hh
 * Author: mikolas
 *
 * Created on January 12, 2011, 4:19 PM
 */
#pragma once
#include "stream_buffer.h"
#include <cstdio>
#include <zlib.h>
class Reader {
  public:
    Reader(gzFile &zf);
    virtual ~Reader();
    [[nodiscard]] int operator*() const { return cur; }
    void operator++() { update_cur(); }
    void skip_whitespace();
    inline size_t get_line_number() const { return lnn; }

    void skip_line();
    void update_cur();

  private:
    size_t lnn;
    StreamBuffer buf;
    int cur;
};

