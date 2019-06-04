//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "portfts.h"

#if !PORTFTS_HAVE_FTS
#include "charset.h"
#include <boost/filesystem.hpp>
#include <string>
#include <memory>
#include <iostream>
namespace fs = boost::filesystem;
namespace sy = boost::system;

struct pFTS {
    fs::recursive_directory_iterator it;
    int flags;
    bool init;
    bool error;
    std::string path;
    pFTSENT ent_buf;
    std::string path_buf;
    std::string name_buf;
};

pFTS *portfts_open(const char *path, int flags)
{
    switch (flags & (pFTS_LOGICAL|pFTS_PHYSICAL)) {
    case pFTS_LOGICAL: case pFTS_PHYSICAL:
        break;
    default:
        return nullptr;
    }

    std::unique_ptr<pFTS> fts(new pFTS);
    fts->flags = flags;
    fts->init = true;
    fts->error = false;
    fts->path.assign(path);
    return fts.release();
}

int portfts_close(pFTS *fts)
{
    delete fts;
    return 0;
}

static int determine_file_type(const fs::path &path, bool logical)
{
    sy::error_code ec;
    fs::file_status status = logical ? fs::status(path, ec) : fs::symlink_status(path, ec);
    if (ec) {
        if (logical && ec == sy::errc::no_such_file_or_directory) {
            ec = sy::error_code();
            status = fs::symlink_status(path, ec);
            if (!ec && status.type() == fs::symlink_file)
                return pFTS_SLNONE;
        }
        if (ec)
            return (ec == sy::errc::no_such_file_or_directory) ? 0 : pFTS_NS;
    }
    switch (status.type()) {
    case fs::directory_file: return pFTS_D;
    case fs::regular_file: return pFTS_F;
    case fs::symlink_file: return pFTS_SL;
    default: return pFTS_DEFAULT;
    }
}

static pFTSENT *build_entry(pFTS *fts, int type, const fs::path &path, int level)
{
    if (type == 0)
        return nullptr;

    fts->ent_buf.fts_info = type;
#ifndef _WIN32
    fts->path_buf = path.native();
    fts->name_buf = path.filename().native();
#else
    if (!convert_utf<wchar_t, char>(path.native(), fts->path_buf, false))
        fts->ent_buf.fts_info = pFTS_ERR;
    if (!convert_utf<wchar_t, char>(path.filename().native(), fts->name_buf, false))
        fts->ent_buf.fts_info = pFTS_ERR;
#endif
    fts->ent_buf.fts_path = fts->path_buf.c_str();
    fts->ent_buf.fts_name = fts->name_buf.c_str();
    fts->ent_buf.fts_level = level;
    return &fts->ent_buf;
}

pFTSENT *portfts_read(pFTS *fts)
{
    sy::error_code ec;

    bool logical = fts->flags & pFTS_LOGICAL;
    fs::symlink_option sym_opt = logical ?
        fs::symlink_option::recurse : fs::symlink_option::none;

    if (fts->error)
        return nullptr;

    if (fts->init) {
        fts->init = false;
        fs::path path = fts->path;
        fts->it = fs::recursive_directory_iterator(path, sym_opt, ec);
        if (ec) {
            fts->error = true;
            return build_entry(fts, determine_file_type(path, logical), path, 0);
        }
        else
            return build_entry(fts, pFTS_D, path, 0);
    }

    pFTSENT *fts_ent = nullptr;
    if (fts->it != fs::recursive_directory_iterator()) {
        const fs::directory_entry &ent = *fts->it;
        const fs::path &path = ent.path();
        fts_ent = build_entry(fts, determine_file_type(path, logical), path, fts->it.level() + 1);

        ec = sy::error_code();
        fts->it.increment(ec);
        if (ec)
            fts->error = true;
    }

    return fts_ent;
}

#endif
