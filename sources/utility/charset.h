#pragma once
#include <gsl.hpp>
#include <string>
#include <memory>

bool to_utf8(gsl::cstring_span src, std::string &dst, const char *src_encoding, bool permissive);

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
