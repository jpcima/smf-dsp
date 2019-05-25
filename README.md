# midiplay
MIDI file player for desktop computer

- The program is not named yet.
- The development is in an early stage, but the program is working well and has many features.
- It works on Linux and Windows. Probably Mac too.

# Keys

The keys are recognized by physical location and not label.

The listed key names are based on the QWERTY keyboard type, it will vary for a different type of layout.

| Key        | Function                                                                     |
|------------|------------------------------------------------------------------------------|
| F2         | Select a MIDI device for playback                                            |
| Tab        | Switch between file browser and track info                                   |
| Esc        | Quit the program                                                             |
| -          | Decrement the proportional window size                                       |
| =          | Increment the proportional window size                                       |
| Page ↑     | Go to previous track in playlist/folder                                      |
| Page ↓     | Go to next track in playlist/folder                                          |
| Space      | Pause or unpause                                                             |
| Home       | Seek to beginning of current track                                           |
| End        | Seek to end of current track                                                 |
| ←↕→        | Navigate in the file browser                                                 |
| ←→         | In track info view, seek track by ± 5 seconds                                |
| Shift + ←→ | In any view, seek track by ± 10 seconds                                      |
| [          | Decrease speed by 1%                                                         |
| ]          | Increase speed by 1%                                                         |
| `          | Switch between repeat modes: On/Off, and Single/Multi                        |
| /          | Scan songs in the current folder of the file browser and play them at random |

# Building

To build the software, type `make` in the source directory.

Install following packages on Debian/LibraZiK/Mint:

- `build-essential` `git` `pkg-config` `libsdl2-dev` `libasound2-dev` `libjack-jackd2-dev` `libuv-dev` `libuchardet-dev`

(in case JACK 1 is preferred over JACK 2, replace `libjack-jackd2-dev` with `libjack-dev`)

Immediately after building, the program is available by starting `./fmidiplay`.

## Building synthesizers

The optional synthesizers have additional requirements, and they are built only if these are present.

- **Fluidsynth**: install the package `libfluidsynth-dev`.
- **FM-OPL3 (ADLMIDI)**: check out external sources into the tree, as follows.  
  `git clone https://github.com/Wohlstand/libADLMIDI.git thirdparty/libADLMIDI`
- **FM-OPN2 (OPNMIDI)**: check out external sources into the tree, as follows.  
  `git clone https://github.com/Wohlstand/libOPNMIDI.git thirdparty/libOPNMIDI`
- **SCC (emidi)**: check out external sources into the tree, as follows.  
  `git clone https://github.com/jpcima/scc.git thirdparty/scc`
- **MT32EMU**: check out external sources into the tree, as follows.  
  `git clone https://github.com/munt/munt.git thirdparty/munt`
