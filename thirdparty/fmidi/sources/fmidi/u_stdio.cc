//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "u_stdio.h"

#if defined(FMIDI_USE_BOOST)
#include <system_error>

FILE_device::FILE_device(FILE *stream, closefn_t closefn)
{
    if (!stream)
        throw std::system_error(EINVAL, std::generic_category());
    impl_.reset(new impl_type{{stream, closefn}});
}

std::streamsize FILE_device::read(char *s, std::streamsize n)
{
    FILE *stream = handle();
    size_t count = ::fread(s, 1, n, stream);
    if (::ferror(stream))
        return -1;
    return count;
}

std::streamsize FILE_device::write(const char *s, std::streamsize n)
{
    FILE *stream = handle();
    size_t count = ::fwrite(s, 1, n, stream);
    if (::ferror(stream))
        return -1;
    return count;
}

std::streampos FILE_device::seek(boost::iostreams::stream_offset off, std::ios::seekdir way)
{
    FILE *stream = handle();
    if (::fseeko(stream, off, way) == -1)
        return -1;
    off_t pos = ::ftello(stream);
    if (pos == -1)
        return -1;
    return std::make_unsigned<off_t>::type(pos);
}

bool FILE_device::flush()
{
    FILE *stream = handle();
    return ::fflush(stream) == 0;
}

#endif  // defined(FMIDI_USE_BOOST)
