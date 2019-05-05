//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "u_memstream.h"

memstream_status memstream::setpos(size_t off)
{
    if (off > length_)
        return ms_err_eof;
    offset_ = off;
    return ms_ok;
}

memstream_status memstream::skip(size_t count)
{
    if (length_ - offset_ < count)
        return ms_err_eof;
    offset_ += count;
    return ms_ok;
}

memstream_status memstream::skipbyte(unsigned byte)
{
    unsigned otherbyte;
    memstream_status status = peekbyte(&otherbyte);
    if (status)
        return status;
    if (byte != otherbyte)
        return ms_err_format;
    ++offset_;
    return ms_ok;
}

const uint8_t *memstream::peek(size_t length)
{
    if (length > length_ - offset_)
        return nullptr;
    return base_ + offset_;
}

const uint8_t *memstream::read(size_t length)
{
    const uint8_t *ptr = peek(length);
    if (ptr)
        offset_ += length;
    return ptr;
}

memstream_status memstream::peekbyte(unsigned *retp)
{
    if (length_ <= offset_)
        return ms_err_eof;
    if (retp)
        *retp = base_[offset_];
    return ms_ok;
}

memstream_status memstream::readbyte(unsigned *retp)
{
    memstream_status ret = peekbyte(retp);
    if (ret)
        return ret;
    ++offset_;
    return ms_ok;
}

memstream_status memstream::readintLE(uint32_t *retp, unsigned length)
{
    const uint8_t *ptr = read(length);
    if (!ptr)
        return ms_err_eof;
    uint32_t ret = 0;
    for (unsigned i = length; i-- > 0;)
        ret = (ret << 8) | ptr[i];
    if (retp)
        *retp = ret;
    return ms_ok;
}

memstream_status memstream::readintBE(uint32_t *retp, unsigned length)
{
    const uint8_t *ptr = read(length);
    if (!ptr)
        return ms_err_eof;
    uint32_t ret = 0;
    for (unsigned i = 0; i < length; ++i)
        ret = (ret << 8) | ptr[i];
    if (retp)
        *retp = ret;
    return ms_ok;
}

memstream_status memstream::readvlq(uint32_t *retp)
{
    memstream_status ret;
    uint32_t value;
    unsigned length;
    std::tie(ret, value, length) = doreadvlq();
    offset_ += length;
    if (retp)
        *retp = value;
    return ret;
}

memstream_status memstream::peekvlq(uint32_t *retp)
{
    memstream_status ret;
    uint32_t value;
    unsigned length;
    std::tie(ret, value, length) = doreadvlq();
    if (retp)
        *retp = value;
    return ret;
}

memstream::vlq_result memstream::doreadvlq()
{
    uint32_t ret = 0;
    unsigned length;
    bool cont = true;
    for (length = 0; cont && length < 4; ++length) {
        if (offset_ + length >= length_)
            return vlq_result{ms_err_eof, 0, 0};
        uint8_t byte = base_[offset_ + length];
        ret = (ret << 7) | (byte & ((1u << 7) - 1));
        cont = byte & (1u << 7);
    }
    if (cont)
        return vlq_result{ms_err_format, 0, 0};
    return vlq_result{ms_ok, ret, length};
}
