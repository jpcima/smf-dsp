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
#include <gsl.hpp>
#include <array>

std::string smf_text_encoding(const fmidi_smf &smf)
{
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

    std::string text;
    text.reserve(1024);

    ///
    const fmidi_event_t *ev;
    fmidi_track_iter_t it;
    fmidi_smf_track_begin(&it, 0);
    while ((ev = fmidi_smf_track_next(&smf, &it)) && ev->type == fmidi_event_meta) {
        uint8_t type = ev->data[0];
        if (type >= 0x01 && type <= 0x05 && ev->datalen - 1 > 0)
            text.append(reinterpret_cast<const char *>(ev->data + 1), ev->datalen - 1);
    }

    if (text.empty())
        return std::string{};

    ///
    for (size_t p = 0; p < num_probers; ++p) {
        nsCharSetProber &prober = *probers[p];
        prober.HandleData(text.data(), text.size());
        detections[p].charset = prober.GetCharSetName();
        detections[p].confidence = prober.GetConfidence();
    }

    ///
    for (size_t p = 0; p < num_probers; ++p) {
        Detection current = detections[p];
        bool valid = current.charset && current.charset[0] != '\0' &&
            has_valid_encoding(text, current.charset);
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
    std::string enc;

    for (size_t p = 0; p < num_probers && enc.empty(); ++p) {
        Detection current = winners[p];
        if (current.charset && current.charset[0] != '\0')
            enc.assign(current.charset);
    }

    return enc;
}

