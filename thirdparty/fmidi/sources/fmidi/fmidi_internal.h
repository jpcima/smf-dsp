//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "fmidi.h"

//------------------------------------------------------------------------------
struct printfmt_quoted {
    printfmt_quoted(const char *text, size_t length)
        : text(text), length(length) {}
    const char *text = nullptr;
    size_t length = 0;
};
std::ostream &operator<<(std::ostream &out, const printfmt_quoted &q);

//------------------------------------------------------------------------------
struct printfmt_bytes {
    printfmt_bytes(const uint8_t *data, size_t size)
        : data(data), size(size) {}
    const uint8_t *data = nullptr;
    size_t size = 0;
};
std::ostream &operator<<(std::ostream &out, const printfmt_bytes &b);

//------------------------------------------------------------------------------
extern thread_local fmidi_error_info_t fmidi_last_error;

#if defined(FMIDI_DEBUG)
# define RET_FAIL(x, e) do {                                      \
        fmidi_error_info_t &fmidi__err = fmidi_last_error;        \
        fmidi__err.file = __FILE__; fmidi__err.line = __LINE__;   \
        fmidi__err.code = (e); return (x); } while (0)
#else
# define RET_FAIL(x, e)                                         \
    do { fmidi_last_error.code = (e); return (x); } while (0)
#endif

//------------------------------------------------------------------------------
#include <vector>
#include <algorithm>
#include <type_traits>
#include <stdio.h>
#include <assert.h>

class Writer {
public:
    virtual ~Writer() {}
    virtual void put(uint8_t byte) = 0;
    virtual void write(const void *data, size_t size) = 0;
    virtual void rwrite(const void *data, size_t size) = 0;
    virtual void writeLE(const void *data, size_t size) = 0;
    virtual void writeBE(const void *data, size_t size) = 0;
    virtual off_t tell() const = 0;
    virtual bool seek(off_t offset, int whence) = 0;
};

template <class T> class WriterT : public Writer {
public:
    virtual ~WriterT() {}
    void rwrite(const void *data, size_t size) override;
    void writeLE(const void *data, size_t size) override;
    void writeBE(const void *data, size_t size) override;
};

class Memory_Writer : public WriterT<Memory_Writer> {
public:
    explicit Memory_Writer(std::vector<uint8_t> &mem)
        : mem(mem), index(mem.size()) {}
    void put(uint8_t byte) override;
    void write(const void *data, size_t size) override;
    off_t tell() const override;
    bool seek(off_t offset, int whence) override;
private:
    std::vector<uint8_t> &mem;
    size_t index = 0;
};

class Stream_Writer : public WriterT<Stream_Writer> {
public:
    explicit Stream_Writer(FILE *stream)
        : stream(stream) {}
    void put(uint8_t byte) override;
    void write(const void *data, size_t size) override;
    off_t tell() const override;
    bool seek(off_t offset, int whence) override;
private:
    FILE *stream = nullptr;
};

//------------------------------------------------------------------------------
union Endian_check {
    uint32_t value;
    uint8_t head_byte;
};

//------------------------------------------------------------------------------
template <class T>
void WriterT<T>::rwrite(const void *data, size_t size)
{
    const uint8_t *bytes = (const uint8_t *)data;
    for (size_t i = size; i-- > 0;)
        static_cast<T *>(this)->put(bytes[i]);
}

template <class T>
void WriterT<T>::writeLE(const void *data, size_t size)
{
    switch(Endian_check{0x11223344}.head_byte) {
    case 0x11: static_cast<T *>(this)->rwrite(data, size); break;
    case 0x44: static_cast<T *>(this)->write(data, size); break;
    default: assert(false);
    }
}

template <class T>
void WriterT<T>::writeBE(const void *data, size_t size)
{
    switch(Endian_check{0x11223344}.head_byte) {
    case 0x11: static_cast<T *>(this)->write(data, size); break;
    case 0x44: static_cast<T *>(this)->rwrite(data, size); break;
    default: assert(false);
    }
}

inline off_t Memory_Writer::tell() const
{
    return index;
}

inline void Stream_Writer::put(uint8_t byte)
{
    fputc(byte, stream);
}

inline void Stream_Writer::write(const void *data, size_t size)
{
    fwrite(data, size, 1, stream);
}

inline bool Stream_Writer::seek(off_t offset, int whence)
{
    return fseek(stream, offset, whence) == 0;
}

inline off_t Stream_Writer::tell() const
{
    return ftell(stream);
}
