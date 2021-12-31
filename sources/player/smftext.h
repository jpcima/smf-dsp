//          Copyright Jean Pierre Cimalando 2019-2022.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <nonstd/string_view.hpp>
#include <string>

struct fmidi_smf;

struct SMF_Encoding_Detector {
public:
    void scan(const fmidi_smf &smf);

    std::string general_encoding() const;
    std::string encoding_for_text(nonstd::string_view input) const;

    std::string decode_to_utf8(nonstd::string_view input) const;

private:
    static nonstd::string_view encoding_from_marker(nonstd::string_view input);
    static nonstd::string_view strip_encoding_marker(nonstd::string_view text, nonstd::string_view enc);

private:
    std::string encoding_;
};
