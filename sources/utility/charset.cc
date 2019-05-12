#include "charset.h"
#include <iconv.h>
#include <bst/utf.hpp>
#include <array>
#include <type_traits>
#include <stdexcept>

#ifdef USE_BOOST_LOCALE
#include <boost/locale/collator.hpp>
#include <locale>
#endif

#ifndef _WIN32
#include "dynamic_icu.h"
#else
#include <windows.h>
#endif

bool to_utf8(gsl::cstring_span src, std::string &dst, const char *src_encoding, bool permissive)
{
    iconv_t cd = iconv_open("UTF-8", src_encoding);
    if (!cd)
        return false;
    auto cd_cleanup = gsl::finally([cd] { iconv_close(cd); });

    size_t src_index = 0;
    size_t src_size = src.size();
    const char *src_start = src.data();

    dst.clear();
    dst.reserve(2 * src_size);

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
        dst.append(b_out, sz_out);
    }

    return true;
}

uint32_t ascii_tolower(uint32_t ch)
{
    return (ch >= U'A' && ch <= U'Z') ? (ch - U'A' + U'a') : ch;
}

uint32_t ascii_toupper(uint32_t ch)
{
    return (ch >= U'a' && ch <= U'z') ? (ch - U'a' + U'A') : ch;
}

#ifndef _WIN32
uint32_t unicode_tolower(uint32_t ch)
{
    static auto f = reinterpret_cast<int32_t (*)(int32_t)>(
        Icuuc_Lib::instance().find_symbol("u_tolower"));
    return f ? f(ch) : ascii_tolower(ch);
}

uint32_t unicode_toupper(uint32_t ch)
{
    static auto f = reinterpret_cast<int32_t (*)(int32_t)>(
        Icuuc_Lib::instance().find_symbol("u_toupper"));
    return f ? f(ch) : ascii_toupper(ch);
}
#else
static uint32_t unicode_w32_map_case(uint32_t in_ch, uint32_t flags)
{
    wchar_t wbuf[4 + 1], *wb, *we;
    wb = wbuf;
    we = bst::utf::utf_traits<wchar_t>::encode(in_ch, wb);
    if (wb == we) return in_ch;
    unsigned cch = LCMapStringW(
        LOCALE_INVARIANT, flags, wb, we - wb, wb, sizeof(wbuf) / sizeof(wbuf[0]));
    if (cch > 0 && wbuf[cch - 1] == L'\0') --cch;
    if (cch == 0) return in_ch;
    wb = wbuf;
    we = wbuf;
    uint32_t out_ch = bst::utf::utf_traits<wchar_t>::decode(we, wb + cch);
    if (wb == we) return in_ch;
    return out_ch;
}

uint32_t unicode_tolower(uint32_t ch)
{
    return unicode_w32_map_case(ch, LCMAP_LOWERCASE);
}

uint32_t unicode_toupper(uint32_t ch)
{
    return unicode_w32_map_case(ch, LCMAP_UPPERCASE);
}
#endif

template <class Compare>
static int utf8_compare_impl(gsl::cstring_span a, gsl::cstring_span b, Compare less)
{
    gsl::cstring_span::iterator a_1 = a.begin(), a_2 = a.end();
    gsl::cstring_span::iterator b_1 = b.begin(), b_2 = b.end();

    while (a_1 != a_2 && b_1 != b_2) {
        if (a_1 == a_2)
            return -1;
        if (b_1 == b_2)
            return +1;
        char32_t cha = bst::utf::utf_traits<char>::decode(a_1, a_2);
        char32_t chb = bst::utf::utf_traits<char>::decode(b_1, b_2);
        if (less(cha, chb))
            return -1;
        if (less(chb, cha))
            return +1;
    }

    return 0;
}

int utf8_compare(gsl::cstring_span a, gsl::cstring_span b)
{
    return utf8_compare_impl(a, b, std::less<char32_t>{});
}

#ifdef USE_BOOST_LOCALE
int utf8_icompare(gsl::cstring_span a, gsl::cstring_span b)
{
    auto &collator = std::use_facet<boost::locale::collator<char>>(std::locale{});
    return collator.compare(collator.primary, a.begin(), a.end(), b.begin(), b.end()) < 0;
}
#else
int utf8_icompare(gsl::cstring_span a, gsl::cstring_span b)
{
    auto compare = [](uint32_t cha, uint32_t chb) -> int
                       { return unicode_tolower(cha) < unicode_tolower(chb); };
    return utf8_compare_impl(a, b, compare);
}
#endif

//
#if 1
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
#else
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
