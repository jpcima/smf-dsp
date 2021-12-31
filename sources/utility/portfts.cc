//          Copyright Jean Pierre Cimalando 2019-2022.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "portfts.h"

#if !PORTFTS_HAVE_FTS
#include <ghc/fs_std.hpp>
#include <string>
#include <memory>
#include <iostream>
#include <system_error>

struct pFTS {
    fs::recursive_directory_iterator it;
    int flags;
    bool init;
    bool error;
    fs::path path;
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
    fts->path = fs::u8path(path);
    return fts.release();
}

int portfts_close(pFTS *fts)
{
    delete fts;
    return 0;
}

static int determine_file_type(const fs::path &path, bool logical)
{
    std::error_code ec;
    fs::file_status status = logical ? fs::status(path, ec) : fs::symlink_status(path, ec);
    if (ec) {
        if (logical && status.type() == fs::file_type::not_found) {
            ec = std::error_code();
            status = fs::symlink_status(path, ec);
            if (!ec && status.type() == fs::file_type::symlink)
                return pFTS_SLNONE;
        }
        if (ec)
            return (status.type() == fs::file_type::not_found) ? 0 : pFTS_NS;
    }
    switch (status.type()) {
    case fs::file_type::directory: return pFTS_D;
    case fs::file_type::regular: return pFTS_F;
    case fs::file_type::symlink: return pFTS_SL;
    default: return pFTS_DEFAULT;
    }
}

static pFTSENT *build_entry(pFTS *fts, int type, const fs::path &path, int level)
{
    if (type == 0)
        return nullptr;

    fts->ent_buf.fts_info = type;
    fts->path_buf = path.generic_u8string();
    fts->name_buf = path.filename().generic_u8string();
    fts->ent_buf.fts_path = fts->path_buf.c_str();
    fts->ent_buf.fts_name = fts->name_buf.c_str();
    fts->ent_buf.fts_level = level;
    return &fts->ent_buf;
}

pFTSENT *portfts_read(pFTS *fts)
{
    std::error_code ec;

    bool logical = fts->flags & pFTS_LOGICAL;
    fs::directory_options sym_opt = logical ?
        fs::directory_options::follow_directory_symlink :
        fs::directory_options::none;

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
        fts_ent = build_entry(fts, determine_file_type(path, logical), path, fts->it.depth() + 1);

        ec = std::error_code();
        fts->it.increment(ec);
        if (ec)
            fts->error = true;
    }

    return fts_ent;
}

#endif
