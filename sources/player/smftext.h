//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <gsl.hpp>
#include <string>

struct fmidi_smf;

struct SMF_Encoding_Detector {
public:
    void scan(const fmidi_smf &smf);

    std::string general_encoding() const;
    std::string encoding_for_text(gsl::cstring_span input) const;

    std::string decode_to_utf8(gsl::cstring_span input) const;

private:
    static gsl::cstring_span encoding_from_marker(gsl::cstring_span input);
    static gsl::cstring_span strip_encoding_marker(gsl::cstring_span text, gsl::cstring_span enc);

private:
    std::string encoding_;
};
