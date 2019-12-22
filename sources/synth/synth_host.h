//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "synth.h"
#include "utility/load_library.h"
#include <SimpleIni.h>
#include <gsl.hpp>
#include <string>
#include <vector>
#include <memory>

class Synth_Host {
public:
    Synth_Host();
    ~Synth_Host();

private:
    Synth_Host(const Synth_Host &) = delete;
    Synth_Host &operator=(const Synth_Host &) = delete;
    Synth_Host(Synth_Host &&) = delete;
    Synth_Host &operator=(Synth_Host &&) = delete;

public:
    struct Plugin_Info {
        std::string id;
        std::string name;
    };

    static const std::string &plugin_dir();
    static const std::vector<Plugin_Info> &plugins();

    bool load(gsl::cstring_span id, double srate);
    void unload();
    void generate(float *buffer, size_t nframes);
    void send_midi(const uint8_t *data, unsigned len);

private:
    Dl_Handle_U module_;
    const synth_interface *intf_ = nullptr;
    synth_object *synth_ = nullptr;

private:
    static std::string plugin_path(const Plugin_Info &info);

private:
    static std::string find_plugin_dir();
    static std::vector<Plugin_Info> do_plugin_scan();
    static void initial_setup_plugin(const Plugin_Info &info, const synth_interface &intf);
    static void initial_setup_synth(const Plugin_Info &info, const synth_interface *intf, synth_object *synth);
};
