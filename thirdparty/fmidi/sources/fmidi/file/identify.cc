//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "fmidi/fmidi.h"
#include "fmidi/fmidi_internal.h"
#include "fmidi/u_stdio.h"
#include <string.h>

fmidi_fileformat_t fmidi_mem_identify(const uint8_t *data, size_t length)
{
    const uint8_t smf_magic[4] = {'M', 'T', 'h', 'd'};
    if (length >= 4 && memcmp(data, smf_magic, 4) == 0)
        return fmidi_fileformat_smf;

    const uint8_t rmi_magic1[4] = {'R', 'I', 'F', 'F'};
    const uint8_t rmi_magic2[8] = {'R', 'M', 'I', 'D', 'd', 'a', 't', 'a'};
    if (length >= 16 && memcmp(data, rmi_magic1, 4) == 0 && memcmp(data + 8, rmi_magic2, 8) == 0)
        return fmidi_fileformat_smf;

    const uint8_t xmi_magic[20] = {
        'F', 'O', 'R', 'M', 0, 0, 0, 14,
        'X', 'D', 'I', 'R', 'I', 'N', 'F', 'O', 0, 0, 0, 2
    };
    if (length >= 20 && memcmp(data, xmi_magic, 20) == 0)
        return fmidi_fileformat_xmi;

    RET_FAIL((fmidi_fileformat_t)-1, fmidi_err_format);
}

fmidi_fileformat_t fmidi_stream_identify(FILE *stream)
{
    rewind(stream);

    uint8_t magic[32];
    size_t size = fread(magic, 1, sizeof(magic), stream);
    if (ferror(stream))
        RET_FAIL((fmidi_fileformat_t)-1, fmidi_err_input);

    return fmidi_mem_identify(magic, size);
}

fmidi_smf_t *fmidi_auto_mem_read(const uint8_t *data, size_t length)
{
    switch (fmidi_mem_identify(data, length)) {
    case fmidi_fileformat_smf:
        return fmidi_smf_mem_read(data, length);
    case fmidi_fileformat_xmi:
        return fmidi_xmi_mem_read(data, length);
    default:
        return nullptr;
    }
}

fmidi_smf_t *fmidi_auto_file_read(const char *filename)
{
    unique_FILE fh(fopen(filename, "rb"));
    if (!fh)
        RET_FAIL(nullptr, fmidi_err_input);

    fmidi_smf_t *smf = fmidi_auto_stream_read(fh.get());
    return smf;
}

fmidi_smf_t *fmidi_auto_stream_read(FILE *stream)
{
    switch (fmidi_stream_identify(stream)) {
    case fmidi_fileformat_smf:
        return fmidi_smf_stream_read(stream);
    case fmidi_fileformat_xmi:
        return fmidi_xmi_stream_read(stream);
    default:
        return nullptr;
    }
}
