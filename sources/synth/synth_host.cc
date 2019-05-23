#include "synth_host.h"
#include "synth_utility.h"
#include "utility/paths.h"
#include <algorithm>
#include <cassert>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#if defined(_WIN32)
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#endif

static const gsl::cstring_span plugin_prefix = "s_";
#if defined(_WIN32)
static const gsl::cstring_span plugin_suffix = ".dll";
#elif defined(__APPLE__)
static const gsl::cstring_span plugin_suffix = ".dylib";
#else
static const gsl::cstring_span plugin_suffix = ".so";
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

bool Synth_Host::load(gsl::cstring_span id, double srate)
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

    Dl_Handle handle(Dl_open(plugin_path(*info).c_str()));
    if (!handle)
        return false;
    Dl_Handle_U handle_u(handle);

    synth_plugin_entry_fn *entry = reinterpret_cast<synth_plugin_entry_fn *>(
        Dl_sym(handle, "synth_plugin_entry"));
    if (!entry)
        return false;

    const synth_interface *intf = entry();
    if (!intf)
        return false;

    unload();

    intf->plugin_init(configuration_dir(*info).c_str());
    module_ = std::move(handle_u);
    intf_ = intf;

    synth_object *synth = intf->synth_instantiate(srate);
    if (!synth)
        return false;

    bool synth_success = false;
    auto synth_failure_cleanup = gsl::finally(
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

    module_.reset();
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

std::string Synth_Host::find_plugin_dir()
{
    std::string dir = get_executable_path();
    while (!dir.empty() && dir.back() != '/')
        dir.pop_back();

    gsl::cstring_span suffix = "/bin/";
    size_t suffixlen = suffix.size();

    if (dir.size() >= suffixlen && gsl::cstring_span(dir).subspan(dir.size() - suffixlen) == suffix) {
        dir.resize(dir.size() - suffixlen);
        dir.append("/lib/" PROGRAM_NAME "/");
    }

    fprintf(stderr, "plugin directory: %s\n", dir.c_str());

    dir.shrink_to_fit();
    return dir;
}

std::vector<Synth_Host::Plugin_Info> Synth_Host::do_plugin_scan()
{
    std::vector<Synth_Host::Plugin_Info> plugins;
    const std::string &dir = plugin_dir();

    DIR *dir_handle = opendir(dir.c_str());
    if (!dir_handle)
        return plugins;
    auto dir_cleanup = gsl::finally([dir_handle] { closedir(dir_handle); });

    while (dirent *ent = readdir(dir_handle)) {
        gsl::cstring_span name = ent->d_name;
        size_t namelen = name.size();

        size_t prefixlen = plugin_prefix.size();
        size_t suffixlen = plugin_suffix.size();

        if (namelen > prefixlen + suffixlen &&
            name.subspan(0, prefixlen) == plugin_prefix &&
            name.subspan(namelen - suffixlen) == plugin_suffix)
        {
            gsl::cstring_span id = name.subspan(prefixlen, namelen - prefixlen - suffixlen);
            Dl_Handle_U handle(Dl_open((dir + name.data()).c_str()));
            synth_plugin_entry_fn *entry = nullptr;
            const synth_interface *intf = nullptr;
            if (handle)
                entry = reinterpret_cast<synth_plugin_entry_fn *>(
                    Dl_sym(handle.get(), "synth_plugin_entry"));
            if (entry)
                intf = entry();
            if (intf) {
                fprintf(stderr, "synth plugin found: %s\n", intf->name);
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

std::unique_ptr<CSimpleIniA> Synth_Host::create_configuration(const Plugin_Info &info)
{
    bool iniIsUtf8 = true, iniMultiKey = true, iniMultiLine = true;
    return std::unique_ptr<CSimpleIniA>(
        new CSimpleIniA(iniIsUtf8, iniMultiKey, iniMultiLine));
}

std::unique_ptr<CSimpleIniA> Synth_Host::load_configuration(const Plugin_Info &info)
{
    FILE *fh = fopen(configuration_path(info).c_str(), "rb");
    if (!fh)
        return nullptr;
    auto fh_cleanup = gsl::finally([fh] { fclose(fh); });

    std::unique_ptr<CSimpleIniA> ini = create_configuration(info);
    if (ini->LoadFile(fh) != SI_OK)
        return nullptr;

    return ini;
}

bool Synth_Host::save_configuration(const Plugin_Info &info, const CSimpleIniA &ini)
{
    FILE *fh = fopen(configuration_path(info).c_str(), "wb");
    if (!fh)
        return false;
    auto fh_cleanup = gsl::finally([fh] { fclose(fh); });

    return ini.SaveFile(fh) == SI_OK;
}

std::string Synth_Host::configuration_path(const Plugin_Info &info)
{
#if defined(_WIN32)
    std::string path = get_executable_path();
    while (!path.empty() && path.back() != '/') path.pop_back();
    path.append("config/");
    mkdir(path.c_str(), 0755);
#else
    std::string path = get_home_directory();
    if (path.empty())
        return std::string();
    path.append(".config/");
    mkdir(path.c_str(), 0755);
    path.append(PROGRAM_DISPLAY_NAME);
    path.push_back('/');
    mkdir(path.c_str(), 0755);
#endif
    path.append("s_");
    path.append(info.id);
    path.append(".ini");
    return path;
}

std::string Synth_Host::configuration_dir(const Plugin_Info &info)
{
    std::string path = configuration_path(info);
    if (path.empty())
        return std::string();
    while (!path.empty() && path.back() != '/') path.pop_back();
    return path;
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
    intf.plugin_init(configuration_dir(info).c_str());
    auto plugin_cleanup = gsl::finally([&intf] { intf.plugin_shutdown(); });

    size_t option_index = 0;
    const synth_option *option = intf.plugin_option(option_index);

    if (!option)
        return;

    std::unique_ptr<CSimpleIniA> ini = load_configuration(info);
    if (!ini) ini = create_configuration(info);
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
        save_configuration(info, *ini);
}

void Synth_Host::initial_setup_synth(const Plugin_Info &info, const synth_interface *intf, synth_object *synth)
{
    std::unique_ptr<CSimpleIniA> ini = load_configuration(info);

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
