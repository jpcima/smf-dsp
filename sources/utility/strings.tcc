//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "strings.h"

template <class Ch>
inline bool string_starts_with(gsl::basic_string_span<const Ch> text, gsl::basic_string_span<const Ch> prefix)
{
    return text.size() >= prefix.size() &&
        std::char_traits<Ch>::compare(text.begin(), prefix.begin(), prefix.size()) == 0;
}

template <class Ch>
inline bool string_ends_with(gsl::basic_string_span<const Ch> text, gsl::basic_string_span<const Ch> suffix)
{
    return text.size() >= suffix.size() &&
        std::char_traits<Ch>::compare(text.end() - suffix.size(), suffix.begin(), suffix.size()) == 0;
}
