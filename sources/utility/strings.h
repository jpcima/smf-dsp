//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <gsl/gsl>

template <class Ch> bool string_starts_with(gsl::basic_string_span<const Ch> text, gsl::basic_string_span<const Ch> prefix);
template <class Ch> bool string_ends_with(gsl::basic_string_span<const Ch> text, gsl::basic_string_span<const Ch> suffix);

#include "strings.tcc"
