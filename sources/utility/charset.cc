#include "charset.h"
#include <iconv.h>
#include <type_traits>
#include <stdexcept>

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
