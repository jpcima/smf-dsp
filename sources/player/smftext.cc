//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "smftext.h"
#include "utility/charset.h"
#include <nsLatin1Prober.h>
#include <nsSJISProber.h>
#include <nsUTF8Prober.h>
#include <fmidi/fmidi.h>
#include <array>
#include <cstring>

void SMF_Encoding_Detector::scan(const fmidi_smf &smf)
{
    std::string &enc = encoding_;
    enc.clear();

    nsLatin1Prober prober_latin1;
    nsSJISProber prober_sjis(PR_TRUE);
    nsUTF8Prober prober_utf8;

    std::array<nsCharSetProber *, 3> probers {
        &prober_latin1,
        &prober_sjis,
        &prober_utf8,
    };

    constexpr size_t num_probers = probers.size();

    ///
    struct Detection {
        const char *charset;
        float confidence;
    };
    std::array<Detection, num_probers> detections;

    std::string full_text;
    full_text.reserve(1024);

    ///
    const fmidi_event_t *ev;
    fmidi_track_iter_t it;
    fmidi_smf_track_begin(&it, 0);
    while ((ev = fmidi_smf_track_next(&smf, &it)) && ev->type == fmidi_event_meta) {
        nonstd::string_view text;

        uint8_t type = ev->data[0];
        if (type >= 0x01 && type <= 0x05 && ev->datalen - 1 > 0)
            text = nonstd::string_view{reinterpret_cast<const char *>(ev->data + 1), ev->datalen - 1};

        // skip detection on text pieces of explicit encoding
        nonstd::string_view explicit_enc = encoding_from_marker(text);
        if (!explicit_enc.empty())
            continue;

        full_text.append(text.data(), text.size());
    }

    if (full_text.empty())
        return;

    ///
    for (size_t p = 0; p < num_probers; ++p) {
        nsCharSetProber &prober = *probers[p];
        prober.HandleData(full_text.data(), full_text.size());
        detections[p].charset = prober.GetCharSetName();
        detections[p].confidence = prober.GetConfidence();
    }

    ///
    for (size_t p = 0; p < num_probers; ++p) {
        Detection current = detections[p];
        bool valid = current.charset && current.charset[0] != '\0' &&
            has_valid_encoding(full_text, current.charset);
        if (!valid && !strcmp(current.charset, "SHIFT_JIS")) {
            // try Microsoft Shift-JIS variant :-)
            if ((valid = has_valid_encoding(full_text, "CP932")))
                detections[p].charset = "CP932";
        }
        if (!valid)
            detections[p].confidence = current.confidence - 0.1f;
    }

    ///
    std::array<Detection, num_probers> winners = detections;
    std::sort(
        winners.begin(), winners.end(),
        [](const Detection &a, const Detection &b) -> bool {
            return a.confidence > b.confidence;
        });

    //
    for (size_t p = 0; p < num_probers && enc.empty(); ++p) {
        Detection current = winners[p];
        if (current.charset && current.charset[0] != '\0')
            enc.assign(current.charset);
    }
}

std::string SMF_Encoding_Detector::general_encoding() const
{
    return encoding_;
}

std::string SMF_Encoding_Detector::encoding_for_text(nonstd::string_view input) const
{
    nonstd::string_view explicit_enc = encoding_from_marker(input);

    if (!explicit_enc.empty())
        return std::string(explicit_enc);

    return encoding_;
}

std::string SMF_Encoding_Detector::decode_to_utf8(nonstd::string_view input) const
{
    std::string output;

    if (!input.empty()) {
        std::string enc = encoding_for_text(input);
        if (enc.empty())
            output = std::string(strip_encoding_marker(input, "UTF-8"));
        else
            to_utf8(strip_encoding_marker(input, enc), output, enc.c_str(), true);
    }

    return output;
}

struct Encoding_Marker {
    nonstd::string_view encoding;
    nonstd::string_view marker;
};

static const std::array<Encoding_Marker, 3> all_encoding_markers {{
    {"UTF-8", "\xef\xbb\xbf"},
    {"UTF-16LE", "\xff\xfe"},
    {"UTF-16BE", "\xfe\xff"},
}};

nonstd::string_view SMF_Encoding_Detector::encoding_from_marker(nonstd::string_view input)
{
    for (const Encoding_Marker &em : all_encoding_markers) {
        if (input.starts_with(em.marker))
            return em.encoding;
    }
    return nonstd::string_view{};
}

nonstd::string_view SMF_Encoding_Detector::strip_encoding_marker(nonstd::string_view text, nonstd::string_view enc)
{
    const Encoding_Marker *em = nullptr;

    for (size_t i = 0; i < all_encoding_markers.size() && !em; ++i) {
        if (all_encoding_markers[i].encoding == enc)
            em = &all_encoding_markers[i];
    }

    if (em && text.starts_with(em->marker))
        text = text.substr(em->marker.size());

    return text;
}
