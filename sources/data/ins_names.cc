/*
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2017-2019 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ins_names.h"
#include "ins_names_data.h"
#include <unordered_map>

static bool mapsInitialized = false;
typedef std::unordered_map<uint32_t, const MidiProgram *> InstrumentMap;
static InstrumentMap XgMap, GsMap, ScMap, Gm2Map, Gm1Map;

static void fillInstMap(InstrumentMap &map, const MidiProgram *pgms, unsigned size)
{
    map.clear();

    for(unsigned i = 0; i < size; ++i)
    {
        MidiProgramId id;
        id.percussive = pgms[i].kind == 'P';
        id.bankMsb = pgms[i].bankMsb;
        id.bankLsb = pgms[i].bankLsb;
        id.program = pgms[i].program;
        const MidiProgram *&pgm_slot = map[id.identifier];
        pgm_slot = &pgms[i];
        // insert pseudo-entry for bank (reserved = 1, program = 0)
        id.reserved = 1;
        id.program = 0;
        const MidiProgram *&bank_slot = map[id.identifier];
        if(!bank_slot)
            bank_slot = &pgms[i];
    }

    //map.shrink_to_fit();
}

static const MidiProgram *lookupMap(const InstrumentMap &map, uint32_t identifier)
{
    InstrumentMap::const_iterator it = map.find(identifier);
    return (it != map.end()) ? it->second : nullptr;
}

const MidiProgram *getMidiProgram(MidiProgramId id, unsigned spec, unsigned *specObtained)
{
    if(!mapsInitialized)
    {
        fillInstMap(XgMap, XgSet, sizeof(XgSet) / sizeof(*XgSet));
        fillInstMap(GsMap, GsSet, sizeof(GsSet) / sizeof(*GsSet));
        fillInstMap(ScMap, ScSet, sizeof(ScSet) / sizeof(*ScSet));
        fillInstMap(Gm2Map, Gm2Set, sizeof(Gm2Set) / sizeof(*Gm2Set));
        fillInstMap(Gm1Map, Gm1Set, sizeof(Gm1Set) / sizeof(*Gm1Set));
        mapsInitialized = true;
    }

    struct MidiSpecToInstrumentMap
    {
        MidiSpec spec;
        const InstrumentMap *map;
    };

    const MidiSpecToInstrumentMap imaps[] =
    {
        {kMidiSpecXG, &XgMap},
        {kMidiSpecGS, &GsMap},
        {kMidiSpecSC, &ScMap},
        {kMidiSpecGM2, &Gm2Map},
        {kMidiSpecGM1, &Gm1Map},
    };

    const MidiProgram *pgm = nullptr;
    unsigned pgmspec = kMidiSpecNone;

    for(const MidiSpecToInstrumentMap &imap : imaps)
    {
        if(!(spec & imap.spec))
            continue;
        pgm = lookupMap(*imap.map, id.identifier);
        if(pgm)
        {
            pgmspec = imap.spec;
            break;
        }
    }

    if(specObtained)
        *specObtained = pgmspec;

    return pgm;
}

const MidiProgram *getFallbackProgram(MidiProgramId id, unsigned spec, unsigned *specObtained)
{
    const MidiProgram *pgm = nullptr;
    if (id.percussive && (id.bankMsb != 0 || id.bankLsb != 0))
    {
        MidiProgramId fallbackId(id.identifier);
        fallbackId.bankMsb = 0;
        fallbackId.bankLsb = 0;
        pgm = getMidiProgram(fallbackId, spec & kMidiSpecGM1, specObtained);
    }

    return pgm;
}

const MidiProgram *getMidiBank(MidiProgramId id, unsigned spec, unsigned *specObtained)
{
    id.reserved = 1;
    id.program = 0;
    return getMidiProgram(id, spec, specObtained);
}
