//          Copyright Jean Pierre Cimalando 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "synth_host.h"
#include "synth_utility.h"
#include "configuration.h"
#include "utility/paths.h"
#include "utility/module.h"
#include "utility/charset.h"
#include "utility/logs.h"
#include <nonstd/scope.hpp>
#include <algorithm>
#include <cassert>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

static const nonstd::string_view plugin_prefix = "s_";
#if defined(_WIN32)
static const nonstd::string_view plugin_suffix = ".dll";
#elif defined(__APPLE__)
static const nonstd::string_view plugin_suffix = ".dylib";
#else
static const nonstd::string_view plugin_suffix = ".so";
#endif

Synth_Host::Synth_Host()
{
}

Synth_Host::~Synth_Host()
{
    unload();
}

const std::string &Synth_Host::plugin_dir()
{
    static const std::string dir = find_plugin_dir();
    return dir;
}

const std::vector<Synth_Host::Plugin_Info> &Synth_Host::plugins()
{
    static const std::vector<Synth_Host::Plugin_Info> plugins = do_plugin_scan();
    return plugins;
}

bool Synth_Host::load(nonstd::string_view id, double srate)
{
    const std::vector<Plugin_Info> &plugin_list = plugins();
    const Plugin_Info *info = nullptr;

    for (size_t i = 0, n = plugin_list.size(); i < n && !info; ++i) {
        const Plugin_Info &cur = plugin_list[i];
        if (cur.id == id)
            info = &cur;
    }

    if (!info)
        return false;

    Dl_Handle handle = [this, &id]() -> Dl_Handle {
        auto it = loaded_modules_.find(std::string(id));
        return (it != loaded_modules_.end()) ? it->second.get() : nullptr;
    }();

    if (!handle) {
        Dl_Handle_U handle_u(Dl_open(plugin_path(*info).c_str()));
        if (!handle_u)
            return false;
        handle = handle_u.get();
        loaded_modules_[std::string(id)] = std::move(handle_u);
    }

    synth_plugin_entry_fn *entry = reinterpret_cast<synth_plugin_entry_fn *>(
        Dl_sym(handle, "synth_plugin_entry"));
    if (!entry)
        return false;

    const synth_interface *intf = entry();
    if (!intf)
        return false;

    unload();

    intf->plugin_init(get_configuration_dir().c_str());
    module_ = handle;
    intf_ = intf;

    synth_object *synth = intf->synth_instantiate(srate);
    if (!synth)
        return false;

    bool synth_success = false;
    auto synth_failure_cleanup = nonstd::make_scope_exit(
        [&synth_success, intf, synth] { if (!synth_success) intf->synth_cleanup(synth); });

    initial_setup_synth(*info, intf, synth);

    if (intf->synth_activate(synth) == -1)
        return false;

    synth_ = synth;
    synth_success = true;
    return true;
}

void Synth_Host::unload()
{
    synth_object *synth = synth_;
    const synth_interface *intf = intf_;

    if (synth) {
        assert(intf);
        intf->synth_cleanup(synth);
        synth_ = nullptr;
    }

    if (intf) {
        intf->plugin_shutdown();
        intf_ = nullptr;
    }

    module_ = nullptr;
}

void Synth_Host::generate(float *buffer, size_t nframes)
{
    synth_object *synth = synth_;
    const synth_interface *intf = intf_;

    if (!synth) {
        std::fill(buffer, buffer + 2 * nframes, 0);
        return;
    }

    assert(intf);
    intf->synth_generate(synth, buffer, nframes);
}

void Synth_Host::send_midi(const uint8_t *data, unsigned len)
{
    synth_object *synth = synth_;
    const synth_interface *intf = intf_;

    if (!synth)
        return;

    assert(intf);
    intf->synth_write(synth, data, len);
}

bool Synth_Host::can_preload() const
{
    synth_object *synth = synth_;
    const synth_interface *intf = intf_;

    if (!synth)
        return false;

    assert(intf);
    return intf->abi_version >= 2 && intf->synth_preload;
}

void Synth_Host::preload(nonstd::span<const synth_midi_ins> instruments)
{
    synth_object *synth = synth_;
    const synth_interface *intf = intf_;

    if (!synth)
        return;

    assert(intf);
    if (intf->abi_version < 2 || !intf->synth_preload)
        return;

    intf->synth_preload(synth, instruments.data(), instruments.size());
}

std::string Synth_Host::find_plugin_dir()
{
    std::string dir = get_executable_path();
    while (!dir.empty() && dir.back() != '/')
        dir.pop_back();

    nonstd::string_view suffix = "/bin/";
    size_t suffixlen = suffix.size();

    if (dir.size() >= suffixlen && nonstd::string_view(dir).substr(dir.size() - suffixlen) == suffix) {
        Log::i("Use FHS plugin search");
        dir.resize(dir.size() - suffixlen);
        dir.append("/lib/" PROGRAM_NAME "/");
    }
    else {
        Log::i("Use portable application plugin search");
    }

    Log::i("Plugin directory: %s", dir.c_str());

    dir.shrink_to_fit();
    return dir;
}

std::vector<Synth_Host::Plugin_Info> Synth_Host::do_plugin_scan()
{
    Log::i("Perform plugin scan");

    std::vector<Synth_Host::Plugin_Info> plugins;
    const std::string &dir = plugin_dir();

    Dir dh(dir);
    if (!dh)
        return plugins;

    for (std::string name; dh.read_next(name);) {
        size_t namelen = name.size();

        size_t prefixlen = plugin_prefix.size();
        size_t suffixlen = plugin_suffix.size();

        if (namelen > prefixlen + suffixlen &&
            nonstd::string_view(name).substr(0, prefixlen) == plugin_prefix &&
            nonstd::string_view(name).substr(namelen - suffixlen) == plugin_suffix)
        {
            nonstd::string_view id = nonstd::string_view(name).substr(prefixlen, namelen - prefixlen - suffixlen);
            Dl_Handle_U handle(Dl_open((dir + name.data()).c_str()));
            synth_plugin_entry_fn *entry = nullptr;
            const synth_interface *intf = nullptr;
            if (handle)
                entry = reinterpret_cast<synth_plugin_entry_fn *>(
                    Dl_sym(handle.get(), "synth_plugin_entry"));
            if (entry)
                intf = entry();
            if (intf) {
                Log::s("Synth plugin found: %s", intf->name);
                Plugin_Info info;
                info.id = std::string(id.data(), id.size());
                info.name = intf->name;
                initial_setup_plugin(info, *intf);
                plugins.emplace_back(std::move(info));
            }
        }
    }

    std::sort(
        plugins.begin(), plugins.end(),
        [](const Plugin_Info &a, const Plugin_Info &b) -> bool { return a.name < b.name; });

    return plugins;
}

std::string Synth_Host::plugin_path(const Plugin_Info &info)
{
    std::string path;
    const std::string &dir = plugin_dir();
    path.reserve(dir.size() + plugin_prefix.size() + info.id.size() + plugin_suffix.size());
    path.append(dir.begin(), dir.end());
    path.append(plugin_prefix.begin(), plugin_prefix.end());
    path.append(info.id);
    path.append(plugin_suffix.begin(), plugin_suffix.end());
    return path;
}

void Synth_Host::initial_setup_plugin(const Plugin_Info &info, const synth_interface &intf)
{
    intf.plugin_init(get_configuration_dir().c_str());
    auto plugin_cleanup = nonstd::make_scope_exit([&intf] { intf.plugin_shutdown(); });

    size_t option_index = 0;
    const synth_option *option = intf.plugin_option(option_index);

    if (!option)
        return;

    std::unique_ptr<CSimpleIniA> ini = load_configuration("s_" + info.id);
    if (!ini) ini = create_configuration();
    bool ini_update = false;

    do {
        if (ini->GetValue("", option->name))
            continue;

        ini_update = true;
        std::string comment = "; " + std::string(option->description);

        switch (option->type) {
        case 'i':
            ini->SetLongValue("", option->name, option->initial.i, comment.c_str());
            break;
        case 'f':
            ini->SetDoubleValue("", option->name, option->initial.f, comment.c_str());
            break;
        case 'b':
            ini->SetBoolValue("", option->name, option->initial.b, comment.c_str());
            break;
        case 's':
            ini->SetValue("", option->name, option->initial.s, comment.c_str());
            break;
        case 'm':
            for (const char *const *p0 = option->initial.m, *const *p = p0; *p; ++p)
                ini->SetValue("", option->name, *p, (p == p0) ? comment.c_str() : nullptr);
            break;
        default:
            assert(false);
        }
    } while ((option = intf.plugin_option(++option_index)));

    if (ini_update)
        save_configuration("s_" + info.id, *ini);
}

void Synth_Host::initial_setup_synth(const Plugin_Info &info, const synth_interface *intf, synth_object *synth)
{
    std::unique_ptr<CSimpleIniA> ini = load_configuration("s_" + info.id);

    if (!ini) {
        const synth_option *opt;
        for (size_t i = 0; (opt = intf->plugin_option(i)); ++i)
            intf->synth_set_option(synth, opt->name, opt->initial);
        return;
    }

    const synth_option *opt;
    for (size_t i = 0; (opt = intf->plugin_option(i)); ++i) {
        synth_value value = {};
        bool valid = false;

        std::unique_ptr<const char *[]> mval;

        switch (opt->type) {
        case 'i':
            if (const char *inival = ini->GetValue("", opt->name)) {
                unsigned count = 0;
                valid = sscanf(inival, "%ld%n", &value.i, &count) == 1 && count == strlen(inival);
            }
            break;
        case 'f':
            if (const char *inival = ini->GetValue("", opt->name)) {
                unsigned count = 0;
                valid = sscanf(inival, "%lf%n", &value.f, &count) == 1 && count == strlen(inival);
            }
            break;
        case 'b':
            if (const char *inival = ini->GetValue("", opt->name)) {
                value.b = !strcmp(inival, "true");
                valid = value.b || !strcmp(inival, "false");
            }
            break;
        case 's':
            if (const char *inival = ini->GetValue("", opt->name)) {
                value.s = inival;
                valid = true;
            }
            break;
        case 'm': {
            CSimpleIniA::TNamesDepend values;
            if (ini->GetAllValues("", opt->name, values)) {
                values.sort(CSimpleIniA::Entry::LoadOrder());
                mval.reset(new const char *[values.size() + 1]);
                const char **p = mval.get();
                for (const CSimpleIniA::Entry &ent : values)
                    *p++ = ent.pItem;
                *p++ = nullptr;
                value.m = mval.get();
                valid = true;
            }
            break;
        }
        default:
            assert(false);
        }

        intf->synth_set_option(synth, opt->name, valid ? value : opt->initial);
    }
}
