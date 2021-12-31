//          Copyright Jean Pierre Cimalando 2019-2022.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#if __cplusplus >= 201703L
#   define maybe_unused [[maybe_unused]]
#elif defined(__GNUC__)
#   define maybe_unused [[gnu::unused]]
#else
#   define maybe_unused
#endif
