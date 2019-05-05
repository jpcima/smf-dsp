//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <memory>
#include <stdio.h>

/////////////////
// FILE DEVICE //
/////////////////

#if defined(FMIDI_USE_BOOST)

#include <boost/iostreams/categories.hpp>
#include <boost/iostreams/positioning.hpp>
#include <ios>

class FILE_device {
public:
    typedef char char_type;
    struct category
        : boost::iostreams::seekable_device_tag,
          boost::iostreams::closable_tag,
          boost::iostreams::flushable_tag,
          boost::iostreams::optimally_buffered_tag {};

private:
    typedef int (*closefn_t)(FILE *);

public:
    explicit FILE_device(FILE *stream, closefn_t closefn = ::fflush);

    FILE *handle() const
        { return impl_ ? impl_->stream_.get() : nullptr; }
    bool is_open() const
        { return handle() != nullptr; }
    void close()
        { impl_.reset(); }

    std::streamsize read(char *s, std::streamsize n);
    std::streamsize write(const char *s, std::streamsize n);

    std::streampos seek(boost::iostreams::stream_offset off, std::ios::seekdir way);
    bool flush();

    std::streamsize optimal_buffer_size() const
        { return 0; }

private:
    struct impl_type { std::unique_ptr<FILE, int(*)(FILE *)> stream_; };
    std::shared_ptr<impl_type> impl_;
};

#include <boost/iostreams/stream.hpp>
typedef boost::iostreams::stream<FILE_device> FILE_stream;
typedef boost::iostreams::stream_buffer<FILE_device> FILE_streambuf;

#endif  // defined(FMIDI_USE_BOOST)

///////////////
// FILE RAII //
///////////////

struct FILE_deleter;
typedef std::unique_ptr<FILE, FILE_deleter> unique_FILE;

struct FILE_deleter {
    void operator()(FILE *stream) const
        { fclose(stream); }
};
