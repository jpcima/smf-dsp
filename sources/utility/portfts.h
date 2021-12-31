//          Copyright Jean Pierre Cimalando 2019-2022.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PORTFTS_HAVE_FTS
#  ifndef _WIN32
#    define PORTFTS_HAVE_FTS 1
#  else
#    define PORTFTS_HAVE_FTS 0
#  endif
#endif

#if !PORTFTS_HAVE_FTS

typedef struct pFTS pFTS;
typedef struct pFTSENT pFTSENT;

#define pFTS_LOGICAL  0x0002 // not recommended; cannot detect cycles without FTS
#define pFTS_PHYSICAL 0x0010

#define pFTS_D 1
#define pFTS_DC 2
#define pFTS_DEFAULT 3
#define pFTS_ERR 7
#define pFTS_F 8
#define pFTS_NS 10
#define pFTS_SL 12
#define pFTS_SLNONE 13

struct pFTSENT {
    int fts_info;
    const char *fts_path;
    const char *fts_name;
    int fts_level;
};

pFTS *portfts_open(const char *path, int flags);
int portfts_close(pFTS *fts);
pFTSENT *portfts_read(pFTS *fts);

#else
#include <sys/types.h>
#include <fts.h>

typedef FTS pFTS;
typedef FTSENT pFTSENT;

#define pFTS_PHYSICAL FTS_PHYSICAL
#define pFTS_LOGICAL FTS_LOGICAL

#define pFTS_D FTS_D
#define pFTS_DC FTS_DC
#define pFTS_DEFAULT FTS_DEFAULT
#define pFTS_ERR FTS_ERR
#define pFTS_F FTS_F
#define pFTS_NS FTS_NS
#define pFTS_SL FTS_SL
#define pFTS_SLNONE FTS_SLNONE

inline pFTS *portfts_open(const char *path, int flags)
{
    char *path_argv[] = {(char *)path, nullptr};
    return fts_open(path_argv, FTS_NOCHDIR|flags, nullptr);
}

inline int portfts_close(pFTS *fts)
{
    return fts_close(fts);
}

inline pFTSENT *portfts_read(pFTS *fts)
{
    return fts_read(fts);
}

#endif

#ifdef __cplusplus
} // extern "C"
#endif
