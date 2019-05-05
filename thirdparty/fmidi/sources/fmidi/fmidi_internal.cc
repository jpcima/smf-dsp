//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "fmidi_internal.h"

thread_local fmidi_error_info_t fmidi_last_error;

fmidi_status_t fmidi_errno()
{
    return fmidi_last_error.code;
}

const fmidi_error_info_t *fmidi_errinfo()
{
    return &fmidi_last_error;
}

const char *fmidi_strerror(fmidi_status_t status)
{
    switch (status) {
    case fmidi_ok: return "success";
    case fmidi_err_format: return "invalid format";
    case fmidi_err_eof: return "premature end of file";
    case fmidi_err_input: return "input error";
    case fmidi_err_largefile: return "file too large";
    case fmidi_err_output: return "output error";
    }
    return nullptr;
}

//------------------------------------------------------------------------------
void Memory_Writer::put(uint8_t byte)
{
    size_t size = mem.size();
    if (index < size)
        mem[index] = byte;
    else
    {
        assert(index == size);
        mem.push_back(byte);
    }
}

void Memory_Writer::write(const void *data, size_t size)
{
    size_t memsize = mem.size();
    size_t index = this->index;

    const uint8_t *bytes = (const uint8_t *)data;
    size_t ncopy = std::min(size, memsize - index);
    std::copy(bytes, bytes + ncopy, &mem[index]);

    mem.insert(mem.end(), bytes + ncopy, bytes + size);
    this->index = index + size;
}

bool Memory_Writer::seek(off_t offset, int whence)
{
    std::make_unsigned<off_t>::type uoffset(offset);
    size_t size = mem.size();
    size_t index = this->index;

    switch (whence) {
    case SEEK_SET:
        if (uoffset > size)
            return false;
        this->index = uoffset;
        break;
    case SEEK_CUR:
        if (offset >= 0) {
            if (size - index < uoffset)
                return false;
            this->index = index + uoffset;
        }
        else {
            if (index < uoffset)
                return false;
            this->index = index - uoffset;
        }
        break;
    case SEEK_END:
        if (uoffset > size)
            return false;
        this->index = size - uoffset;
        break;
    }
    return true;
}
