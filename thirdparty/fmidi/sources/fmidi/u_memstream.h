//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <tuple>

enum memstream_status {  // make it match fmidi status codes
    ms_ok,
    ms_err_format,
    ms_err_eof,
};

class memstream {
public:
    memstream(const uint8_t *data, size_t length);

    size_t endpos() const;
    size_t getpos() const;
    memstream_status setpos(size_t off);

    memstream_status skip(size_t count);
    memstream_status skipbyte(unsigned byte);

    const uint8_t *peek(size_t length);
    const uint8_t *read(size_t length);

    memstream_status peekbyte(unsigned *retp);
    memstream_status readbyte(unsigned *retp);

    memstream_status readintLE(uint32_t *retp, unsigned length);
    memstream_status readintBE(uint32_t *retp, unsigned length);
    memstream_status readvlq(uint32_t *retp);
    memstream_status peekvlq(uint32_t *retp);

private:
    const uint8_t *base_ = nullptr;
    size_t length_ = 0;
    size_t offset_ = 0;
    typedef std::tuple<memstream_status, uint32_t, unsigned> vlq_result;
    vlq_result doreadvlq();
};

//------------------------------------------------------------------------------
inline memstream::memstream(const uint8_t *data, size_t length)
    : base_(data), length_(length) {
}

inline size_t memstream::endpos() const {
    return length_;
}

inline size_t memstream::getpos() const {
    return offset_;
}
