#pragma once
#include <gsl.hpp>
#include <string>
#include <memory>

// conversion
bool to_utf8(gsl::cstring_span src, std::string &dst, const char *src_encoding, bool permissive);
uint32_t ascii_tolower(uint32_t ch);
uint32_t ascii_toupper(uint32_t ch);
uint32_t unicode_tolower(uint32_t ch);
uint32_t unicode_toupper(uint32_t ch);

// comparison
int utf8_compare(gsl::cstring_span a, gsl::cstring_span b);
int utf8_icompare(gsl::cstring_span a, gsl::cstring_span b);

// detection
class Encoding_Detector
{
public:
    Encoding_Detector();
    ~Encoding_Detector();
    void start();
    void finish();
    void feed(gsl::cstring_span text);
    const char *detected_encoding();

private:
    struct Impl;
    std::unique_ptr<Impl> P;
};
