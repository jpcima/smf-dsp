//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <cstdint>

#pragma pack(push, 1)
struct Midi_Program {
    //! Kind of instrument. 'M' melodic 'P' percussive
    char kind;
    //! Bank identifier MSB
    unsigned char bank_msb;
    //! Bank identifier LSB. (if percussive, this is the program number)
    unsigned char bank_lsb;
    //! Program number (if percussive, this is the key number)
    unsigned char program;
    //! Bank name
    const char *bank_name;
    //! Patch name
    const char *patch_name;
};
#pragma pack(pop)

enum Midi_Spec {
    //! General MIDI Level 1
    Midi_Spec_GM1 = 1,
    //! General MIDI Level 2
    Midi_Spec_GM2 = 2,
    //! Roland Sound Canvas
    Midi_Spec_SC  = 4,
    //! Roland GS
    Midi_Spec_GS  = 8,
    //! Yamaha XG Level 1, 2, 3
    Midi_Spec_XG  = 16,
    //! No MIDI specification
    Midi_Spec_None = 0,
    //! Any MIDI specification
    Midi_Spec_Any = 255,
};

struct Midi_Program_Id {
    explicit Midi_Program_Id(uint32_t i = 0) : identifier(i)
    {
    }

    Midi_Program_Id(bool d, unsigned m, unsigned l, unsigned p) : identifier()
    {
        percussive = d;
        bank_msb = m;
        bank_lsb = l;
        program = p;
    }

    union {
        uint32_t identifier;
        struct {
            uint32_t percussive : 1;
            uint32_t bank_msb : 7;
            uint32_t bank_lsb : 7;
            uint32_t program : 7;
            uint32_t reserved : 1;
        };
    };
};

namespace Midi_Data {
const Midi_Program *get_program(Midi_Program_Id id, unsigned spec, unsigned *spec_obtained = nullptr);
const Midi_Program *get_fallback_program(Midi_Program_Id id, unsigned spec, unsigned *spec_obtained = nullptr);
const Midi_Program *get_bank(Midi_Program_Id id, unsigned spec, unsigned *spec_obtained = nullptr);
} // namespace Midi_Data
