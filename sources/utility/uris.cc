//          Copyright Jean Pierre Cimalando 2019-2022.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "uris.h"
#include <nonstd/scope.hpp>

#if defined(HAVE_GLIB_GURI)
#include <glib.h>

bool parse_uri(const char *text, Uri_Split &result)
{
    GUri *uri = g_uri_parse(text, G_URI_FLAGS_NONE, nullptr);
    if (!uri)
        return false;

    auto safe_string = [](const char *p) -> std::string {
        return p ? std::string(p) : std::string();
    };

    auto uri_cleanup = nonstd::make_scope_exit([uri]() { g_uri_unref(uri); });

    result.scheme = safe_string(g_uri_get_scheme(uri));
    result.user = safe_string(g_uri_get_user(uri));
    result.password = safe_string(g_uri_get_password(uri));
    result.host = safe_string(g_uri_get_host(uri));
    result.port = g_uri_get_port(uri);
    result.path = safe_string(g_uri_get_path(uri));
    result.query = safe_string(g_uri_get_query(uri));
    result.fragment = safe_string(g_uri_get_fragment(uri));

    return true;
}
#elif defined(HAVE_APR)
#include <apr_uri.h>

bool parse_uri(const char *text, Uri_Split &result)
{
    apr_pool_t *pool = nullptr;
    if (apr_pool_create(&pool, nullptr) != APR_SUCCESS)
        throw std::bad_alloc();
    auto pool_cleanup = nonstd::make_scope_exit([pool]() { apr_pool_destroy(pool); });

    apr_uri_t uri {};
    if (apr_uri_parse(pool, text, &uri) != APR_SUCCESS)
        return false;

    auto safe_string = [](const char *p) -> std::string {
        return p ? std::string(p) : std::string();
    };

    result.scheme = safe_string(uri.scheme);
    result.user = safe_string(uri.user);
    result.password = safe_string(uri.password);
    result.host = safe_string(uri.hostname);
    result.port = uri.port_str ? uri.port : -1;
    result.path = safe_string(uri.path);
    result.query = safe_string(uri.query);
    result.fragment = safe_string(uri.fragment);

    return true;
}
#endif
