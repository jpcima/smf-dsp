//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "charset.h"
#include <iconv.h>
#include <unicode/uchar.h>
#include <unicode/ucol.h>
#include <unicode/uiter.h>
#include <unicode/unorm2.h>
#include <utf/utf.hpp>
#include <array>
#include <type_traits>
#include <stdexcept>
#include <cerrno>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

static bool to_utf8_impl(gsl::cstring_span src, std::string *dst, const char *src_encoding, bool permissive)
{
    iconv_t cd = iconv_open("UTF-8", src_encoding);
    if (!cd)
        return false;
    auto cd_cleanup = gsl::finally([cd] { iconv_close(cd); });

    size_t src_index = 0;
    size_t src_size = src.size();
    const char *src_start = src.data();

    if (dst) {
        dst->clear();
        dst->reserve(2 * src_size);
    }

    while (src_index < src_size) {
        char b_out[4];

        char *p_in = const_cast<char *>(src_start) + src_index;
        char *p_out = b_out;
        size_t sz_in = src_size - src_index;
        size_t sz_out = sizeof(b_out);

        iconv(cd, &p_in, &sz_in, &p_out, &sz_out);
        sz_in = src_size - src_index - sz_in;
        sz_out = sizeof(b_out) - sz_out;

        if (sz_in == 0) {
            if (!permissive)
                return false;
            sz_in = 1;
        }

        src_index += sz_in;
        if (dst)
            dst->append(b_out, sz_out);
    }

    return true;
}

bool to_utf8(gsl::cstring_span src, std::string &dst, const char *src_encoding, bool permissive)
{
    return to_utf8_impl(src, &dst, src_encoding, permissive);
}

bool has_valid_encoding(gsl::cstring_span src, const char *src_encoding)
{
    return to_utf8_impl(src, nullptr, src_encoding, false);
}

template <class CharSrc, class CharDst> bool convert_utf(gsl::basic_string_span<const CharSrc> src, std::basic_string<CharDst> &dst, bool permissive)
{
    typedef utf::utf_traits<CharSrc> Src;
    typedef utf::utf_traits<CharDst> Dst;

    dst.clear();
    dst.reserve(src.size() * Dst::max_width);

    for (auto si = src.begin(), se = src.end(); si != se;) {
        utf::code_point cp = Src::decode(si, se);
        if (cp == utf::incomplete || cp == utf::illegal) {
            if (!permissive)
                return false;
        }
        else {
            CharDst db[Dst::max_width], *de = Dst::encode(cp, db);
            dst.append(db, de - db);
        }
    }

    return true;
}

template bool convert_utf(gsl::basic_string_span<const wchar_t>, std::basic_string<char> &, bool);
template bool convert_utf(gsl::basic_string_span<const char16_t>, std::basic_string<char> &, bool);
template bool convert_utf(gsl::basic_string_span<const char32_t>, std::basic_string<char> &, bool);
template bool convert_utf(gsl::basic_string_span<const char>, std::basic_string<wchar_t> &, bool);
template bool convert_utf(gsl::basic_string_span<const char>, std::basic_string<char16_t> &, bool);
template bool convert_utf(gsl::basic_string_span<const char>, std::basic_string<char32_t> &, bool);

uint32_t unicode_tolower(uint32_t ch)
{
    return u_tolower(ch);
}

uint32_t unicode_toupper(uint32_t ch)
{
    return u_toupper(ch);
}

static const UNormalizer2 *NFD_normalizer()
{
    static const UNormalizer2 *norm = []() -> const UNormalizer2 * {
        UErrorCode status = U_ZERO_ERROR;
        const UNormalizer2 *norm = unorm2_getNFDInstance(&status);
        return U_SUCCESS(status) ? norm : nullptr;
    }();
    return norm;
};

uint32_t unicode_nfd_base(uint32_t ch)
{
    static const UNormalizer2 *norm = NFD_normalizer();
    UChar dec[8];
    UErrorCode status = U_ZERO_ERROR;
    int32_t declen = unorm2_getDecomposition(
        norm, ch, dec, sizeof(dec) / sizeof(dec[0]), &status);
    UChar32 dc = U_SENTINEL;
    if (declen > 0) {
        UCharIterator iter;
        uiter_setString(&iter, dec, declen);
        dc = uiter_current32(&iter);
    }
    return (dc != U_SENTINEL) ? dc : 0;
}

///
struct UCollator_delete {
    void operator()(UCollator *x) const noexcept { ucol_close(x); }
};
typedef std::unique_ptr<UCollator, UCollator_delete> UCollator_u;

///
int utf8_strcoll(gsl::cstring_span a, gsl::cstring_span b)
{
    static UCollator_u coll(
        []() -> UCollator * {
            UErrorCode status = U_ZERO_ERROR;
            UCollator *coll = ucol_open(uloc_getDefault(), &status);
            if (U_FAILURE(status))
                throw std::runtime_error(u_errorName(status));
            return coll;
        }());

    UErrorCode status = U_ZERO_ERROR;
    return ucol_strcollUTF8(
        coll.get(), a.data(), a.size(), b.data(), b.size(), &status);
}

//
FILE *fopen_utf8(const char *path, const char *mode)
{
#ifndef _WIN32
    return fopen(path, mode);
#else
    std::wstring wpath, wmode;
    if (!convert_utf<char, wchar_t>(path, wpath, false) ||
        !convert_utf<char, wchar_t>(mode, wmode, false))
    {
        errno = EINVAL;
        return nullptr;
    }
    return _wfopen(wpath.c_str(), wmode.c_str());
#endif
}

int filemode_utf8(const char *path)
{
#ifndef _WIN32
    struct stat st;
    if (stat(path, &st) != 0)
        return -1;
#else
    struct _stat st;
    std::wstring wpath;
    if (!convert_utf<char, wchar_t>(path, wpath, false)) {
        errno = EINVAL;
        return -1;
    }
    if (_wstat(wpath.c_str(), &st) != 0)
        return -1;
#endif
    return st.st_mode;
}

//
Dir::Dir(const gsl::cstring_span path)
{
#ifndef _WIN32
    std::string cpath(path.begin(), path.end());
    dh_.reset((DirInfo *)opendir(cpath.c_str()));
#else
    std::wstring wpath;
    if (convert_utf<char, wchar_t>(path, wpath, false))
        dh_.reset((DirInfo *)_wopendir(wpath.c_str()));
#endif
}

Dir::~Dir()
{
}

#ifndef _WIN32
void Dir::DirInfo_delete::operator()(DirInfo *dh) const noexcept { closedir((DIR *)dh); }
#else
void Dir::DirInfo_delete::operator()(DirInfo *dh) const noexcept { _wclosedir((_WDIR *)dh); }
#endif

bool Dir::read_next(std::string &name)
{
#ifndef _WIN32
    dirent *ent = readdir((DIR *)dh_.get());
    if (!ent)
        return false;
    name.assign(&ent->d_name[0]);
    return true;
#else
    do {
        _wdirent *ent = _wreaddir((_WDIR *)dh_.get());
        if (!ent)
            return false;
        if (convert_utf<wchar_t, char>(&ent->d_name[0], name, false))
            return true;
    } while (1);
#endif
}

#ifndef _WIN32
int Dir::fd()
{
    return dirfd((DIR *)dh_.get());
}
#endif

//
#if 0
#include <uchardet/uchardet.h>

struct Encoding_Detector::Impl {
    struct uchardet_deleter { void operator()(uchardet_t x) const noexcept { uchardet_delete(x); } };
    std::unique_ptr<std::remove_pointer<uchardet_t>::type, uchardet_deleter> ud_;
};

Encoding_Detector::Encoding_Detector()
    : P(new Impl)
{
    uchardet_t ud = uchardet_new();
    if (!ud)
        throw std::bad_alloc();
    P->ud_.reset(ud);
}

Encoding_Detector::~Encoding_Detector()
{
}

void Encoding_Detector::start()
{
    uchardet_reset(P->ud_.get());
}

void Encoding_Detector::finish()
{
    uchardet_data_end(P->ud_.get());
}

void Encoding_Detector::feed(gsl::cstring_span text)
{
    uchardet_handle_data(P->ud_.get(), text.data(), text.size());
}

const char *Encoding_Detector::detected_encoding()
{
    return uchardet_get_charset(P->ud_.get());
}

//
#elif 0
#include <unicode/ucsdet.h>

struct Encoding_Detector::Impl {
    icu::LocalUCharsetDetectorPointer det_;
    std::string text_;
};

Encoding_Detector::Encoding_Detector()
    : P(new Impl)
{
    P->text_.reserve(1024);
    start();
}

Encoding_Detector::~Encoding_Detector()
{
}

void Encoding_Detector::start()
{
    P->text_.clear();

    UErrorCode status = U_ZERO_ERROR;
    UCharsetDetector *det = ucsdet_open(&status);
    if (status != U_ZERO_ERROR)
        throw std::runtime_error("ucsdet_open");
    P->det_.adoptInstead(det);
}

void Encoding_Detector::finish()
{
    UErrorCode status = U_ZERO_ERROR;
    ucsdet_setText(P->det_.getAlias(), P->text_.data(), P->text_.size(), &status);
    if (status != U_ZERO_ERROR)
        throw std::runtime_error("ucsdet_setText");
}

void Encoding_Detector::feed(gsl::cstring_span text)
{
    P->text_.append(text.data(), text.size());
}

const char *Encoding_Detector::detected_encoding()
{
    UErrorCode status = U_ZERO_ERROR;
    const UCharsetMatch *match = ucsdet_detect(P->det_.getAlias(), &status);
    if (status != U_ZERO_ERROR)
        throw std::runtime_error("ucsdet_detect");

    if (!match)
        return nullptr;

    const char *name = ucsdet_getName(match, &status);
    if (status != U_ZERO_ERROR)
        throw std::runtime_error("ucsdet_getName");

    return name;
}

#endif
