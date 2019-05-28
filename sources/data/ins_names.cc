//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "ins_names.h"
#include "ins_names_data.h"
#include <unordered_map>

namespace Midi_Data {

static bool maps_initialized = false;
typedef std::unordered_map<uint32_t, const Midi_Program *> Instrument_Map;
static Instrument_Map XgMap, GsMap, ScMap, Gm2Map, Gm1Map;

static void fill_inst_map(Instrument_Map &map, const Midi_Program *pgms, unsigned size)
{
    map.clear();

    for (unsigned i = 0; i < size; ++i) {
        Midi_Program_Id id;
        id.percussive = pgms[i].kind == 'P';
        id.bank_msb = pgms[i].bank_msb;
        id.bank_lsb = pgms[i].bank_lsb;
        id.program = pgms[i].program;
        const Midi_Program *&pgm_slot = map[id.identifier];
        pgm_slot = &pgms[i];
        // insert pseudo-entry for bank (reserved = 1, program = 0)
        id.reserved = 1;
        id.program = 0;
        const Midi_Program *&bank_slot = map[id.identifier];
        if (!bank_slot)
            bank_slot = &pgms[i];
    }

    //map.shrink_to_fit();
}

static const Midi_Program *lookup_map(const Instrument_Map &map, uint32_t identifier)
{
    Instrument_Map::const_iterator it = map.find(identifier);
    return (it != map.end()) ? it->second : nullptr;
}

const Midi_Program *get_program(Midi_Program_Id id, unsigned spec, unsigned *specObtained)
{
    if (!maps_initialized) {
        fill_inst_map(XgMap, XgSet, sizeof(XgSet) / sizeof(*XgSet));
        fill_inst_map(GsMap, GsSet, sizeof(GsSet) / sizeof(*GsSet));
        fill_inst_map(ScMap, ScSet, sizeof(ScSet) / sizeof(*ScSet));
        fill_inst_map(Gm2Map, Gm2Set, sizeof(Gm2Set) / sizeof(*Gm2Set));
        fill_inst_map(Gm1Map, Gm1Set, sizeof(Gm1Set) / sizeof(*Gm1Set));
        maps_initialized = true;
    }

    struct MidiSpecToInstrumentMap {
        Midi_Spec spec;
        const Instrument_Map *map;
    };

    const MidiSpecToInstrumentMap imaps[] = {
        {Midi_Spec_XG, &XgMap},
        {Midi_Spec_GS, &GsMap},
        {Midi_Spec_SC, &ScMap},
        {Midi_Spec_GM2, &Gm2Map},
        {Midi_Spec_GM1, &Gm1Map},
    };

    const Midi_Program *pgm = nullptr;
    unsigned pgmspec = Midi_Spec_None;

    for (const MidiSpecToInstrumentMap &imap : imaps) {
        if (!(spec & imap.spec))
            continue;
        pgm = lookup_map(*imap.map, id.identifier);
        if (pgm) {
            pgmspec = imap.spec;
            break;
        }
    }

    if (specObtained)
        *specObtained = pgmspec;

    return pgm;
}

const Midi_Program *get_fallback_program(Midi_Program_Id id, unsigned spec, unsigned *specObtained)
{
    const Midi_Program *pgm = nullptr;
    if (id.percussive && (id.bank_msb != 0 || id.bank_lsb != 0)) {
        Midi_Program_Id fallbackId(id.identifier);
        fallbackId.bank_msb = 0;
        fallbackId.bank_lsb = 0;
        pgm = get_program(fallbackId, spec & Midi_Spec_GM1, specObtained);
    }

    return pgm;
}

const Midi_Program *get_bank(Midi_Program_Id id, unsigned spec, unsigned *specObtained)
{
    id.reserved = 1;
    id.program = 0;
    return get_program(id, spec, specObtained);
}

} // namespace Midi_Data
