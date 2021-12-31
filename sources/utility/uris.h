//          Copyright Jean Pierre Cimalando 2019-2022.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <string>

struct Uri_Split {
    std::string scheme;
    std::string user;
    std::string password;
    std::string host;
    int port = -1;
    std::string path;
    std::string query;
    std::string fragment;
};

bool parse_uri(const char *text, Uri_Split &result);
