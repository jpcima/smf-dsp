//          Copyright Jean Pierre Cimalando 2019-2022.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#if defined(HAVE_MPRIS)
#include "mpris_server.h"
#include "application.h"
#include "player/state.h"
#include "player/playlist.h"
#include "utility/paths.h"
#include "utility/uris.h"
#include "utility/logs.h"
#include "org.mpris.MediaPlayer2.h"
#include "org.mpris.MediaPlayer2.Player.h"
#include <gio/gio.h>
#include <glib.h>
#include <unistd.h>
#include <nonstd/scope.hpp>
#include <type_traits>
#include <functional>
#include <string>
#include <vector>
#include <cstring>
#include <cinttypes>

static constexpr bool mpris_verbose = false;

struct Mpris_Server::Impl
{
    explicit Impl(Application &app);
    ~Impl();

    Application *app_;
    Player_State last_state_;

    OrgMprisMediaPlayer2 *mpris_ = nullptr;
    OrgMprisMediaPlayer2Player *player_ = nullptr;

    GMainLoop *loop_ = nullptr;
    guint connection_id_ = 0;
    bool connection_over_ = false;

    void init_properties();
    void init_methods();

    struct PropertySlot {
        Impl *self {};
        gpointer object {};
        gulong connection {};
        void (Impl:: *callback)(GValue *) {};
    };
    std::vector<std::unique_ptr<PropertySlot>> property_slots_;

    void install_property_change_callback(gpointer object, const gchar *prop_name, void (Impl:: *callback)(GValue *));
    void enable_property_change_callbacks(bool enable);

    static void on_bus_acquired(GDBusConnection *connection, const gchar *name, gpointer user_data);
    static void on_name_acquired(GDBusConnection *connection, const gchar *name, gpointer user_data);
    static void on_name_lost(GDBusConnection *connection, const gchar *name, gpointer user_data);

    // helpers
    static const std::string &get_track_id_prefix();
    static std::string make_track_id(uint64_t serial);
    static bool parse_track_id(const gchar *id, uint64_t &serial);

    // org.mpris.MediaPlayer2
    static gboolean handle_quit (
        OrgMprisMediaPlayer2 *object,
        GDBusMethodInvocation *invocation);
    static gboolean handle_raise (
        OrgMprisMediaPlayer2 *object,
        GDBusMethodInvocation *invocation);

    // org.mpris.MediaPlayer2.Player
    static gboolean handle_next (
        OrgMprisMediaPlayer2Player *object,
        GDBusMethodInvocation *invocation);
    static gboolean handle_open_uri (
        OrgMprisMediaPlayer2Player *object,
        GDBusMethodInvocation *invocation,
        const gchar *arg_Uri);
    static gboolean handle_pause (
        OrgMprisMediaPlayer2Player *object,
        GDBusMethodInvocation *invocation);
    static gboolean handle_play (
        OrgMprisMediaPlayer2Player *object,
        GDBusMethodInvocation *invocation);
    static gboolean handle_play_pause (
        OrgMprisMediaPlayer2Player *object,
        GDBusMethodInvocation *invocation);
    static gboolean handle_previous (
        OrgMprisMediaPlayer2Player *object,
        GDBusMethodInvocation *invocation);
    static gboolean handle_seek (
        OrgMprisMediaPlayer2Player *object,
        GDBusMethodInvocation *invocation,
        gint64 arg_Offset);
    static gboolean handle_set_position (
        OrgMprisMediaPlayer2Player *object,
        GDBusMethodInvocation *invocation,
        const gchar *arg_TrackId,
        gint64 arg_Position);
    static gboolean handle_stop (
        OrgMprisMediaPlayer2Player *object,
        GDBusMethodInvocation *invocation);

    void loop_status_changed(GValue *value);
    void rate_changed(GValue *value);
    void volume_changed(GValue *value);
};

Mpris_Server::Mpris_Server(Application &app)
    : impl_(new Impl(app))
{
    Impl &impl = *impl_;

    impl.mpris_ = org_mpris_media_player2_skeleton_new();
    impl.player_ = org_mpris_media_player2_player_skeleton_new();

    for (GObject *obj : {G_OBJECT(impl.mpris_), G_OBJECT(impl.player_)})
        g_object_set_data(obj, "my:self", &impl);

    impl.init_properties();
    impl.init_methods();

    impl.loop_ = g_main_loop_new(nullptr, FALSE);

    const std::string bus_name = "org.mpris.MediaPlayer2.smf_dsp.instance" + std::to_string(getpid());

    impl.connection_id_ = g_bus_own_name(
        G_BUS_TYPE_SESSION, bus_name.c_str(), G_BUS_NAME_OWNER_FLAGS_NONE,
        &Impl::on_bus_acquired, &Impl::on_name_acquired, &Impl::on_name_lost, &impl, nullptr);
}

Mpris_Server::~Mpris_Server()
{
}

void Mpris_Server::exec()
{
    Impl &impl = *impl_;

    if (!impl.connection_over_) {
        GMainContext *ctx = g_main_loop_get_context(impl.loop_);
        while (g_main_context_iteration(ctx, false));
    }
    else if (impl.connection_id_ != 0) {
        g_bus_unown_name(impl.connection_id_);
        impl.connection_id_ = 0;
    }
}

void Mpris_Server::receive_new_player_state(const Player_State &ps)
{
    OrgMprisMediaPlayer2Player *player = impl_->player_;
    Player_State &lastps = impl_->last_state_;

    impl_->enable_property_change_callbacks(false);

    if (ps.status != lastps.status) {
        const gchar *playback_status =
            (ps.status == Playing_Status::Playing) ? "Playing" :
            (ps.status == Playing_Status::Paused) ? "Paused" : "Stopped";
        org_mpris_media_player2_player_set_playback_status(player, playback_status);
    }
    if (ps.repeat_mode != lastps.repeat_mode) {
        const gchar *loop_status =
            ((ps.repeat_mode & (Repeat_On|Repeat_Off)) != Repeat_On) ? "None" :
            ((ps.repeat_mode & (Repeat_Multi|Repeat_Single)) != Repeat_Multi) ? "Track" :
            "Playlist";
        org_mpris_media_player2_player_set_loop_status(player, loop_status);
    }
    if (ps.speed != lastps.speed) {
        double rate = ps.speed / 100.0;
        org_mpris_media_player2_player_set_rate(player, rate);
    }

    //org_mpris_media_player2_player_set_shuffle(player, );

    if (ps.song_serial != lastps.song_serial) {
        GVariantBuilder bld {};
        g_variant_builder_init(&bld, G_VARIANT_TYPE("a{sv}"));
        if (ps.status == Playing_Status::Stopped) {
            g_variant_builder_add(&bld, "{sv}", "mpris:trackid", g_variant_new_string("/org/mpris/MediaPlayer2/TrackList/NoTrack"));
        }
        else {
            g_variant_builder_add(&bld, "{sv}", "mpris:trackid", g_variant_new_object_path(Impl::make_track_id(ps.song_serial).c_str()));
            g_variant_builder_add(&bld, "{sv}", "mpris:length", g_variant_new_int64((uint64_t)(ps.duration * 1e6)));
            if (!ps.song_metadata.name.empty())
                g_variant_builder_add(&bld, "{sv}", "xesam:title", g_variant_new_string(ps.song_metadata.name.c_str()));
            else
                g_variant_builder_add(&bld, "{sv}", "xesam:title", g_variant_new_string(std::string(path_file_name(ps.file_path)).c_str()));
            if (!ps.song_metadata.author.empty())
                g_variant_builder_add(&bld, "{sv}", "xesam:artist", g_variant_new_string(ps.song_metadata.author.c_str()));
        }
        GVariant *value = g_variant_builder_end(&bld);
        org_mpris_media_player2_player_set_metadata(player, value);
    }
    if (ps.volume != lastps.volume)
        org_mpris_media_player2_player_set_volume(player, ps.volume);
    if (ps.time_position != lastps.time_position) {
        gint64 position = (gint64)(ps.time_position * 1e6);
        org_mpris_media_player2_player_set_position(player, position);
    }

    if (ps.seek_serial != lastps.seek_serial) {
        gint64 position = (gint64)(ps.time_position * 1e6);
        org_mpris_media_player2_player_emit_seeked(player, position);
    }

    impl_->enable_property_change_callbacks(true);

    lastps = ps;
}

//------------------------------------------------------------------------------
Mpris_Server::Impl::Impl(Application &app)
{
    app_ = &app;
}

Mpris_Server::Impl::~Impl()
{
    if (connection_id_ != 0)
        g_bus_unown_name(connection_id_);
    if (loop_)
        g_main_loop_unref(loop_);
    if (mpris_)
        g_object_unref(mpris_);
    if (player_)
        g_object_unref(player_);
}

void Mpris_Server::Impl::init_properties()
{
    GVariantBuilder bld {};
    GVariant *value;

    org_mpris_media_player2_set_can_quit(mpris_, true);
    org_mpris_media_player2_set_fullscreen(mpris_, false);
    org_mpris_media_player2_set_can_set_fullscreen(mpris_, false);
    org_mpris_media_player2_set_can_raise(mpris_, true);
    org_mpris_media_player2_set_has_track_list(mpris_, false);
    org_mpris_media_player2_set_identity(mpris_, PROGRAM_DISPLAY_NAME);
    org_mpris_media_player2_set_desktop_entry(mpris_, PROGRAM_NAME);
    org_mpris_media_player2_set_supported_uri_schemes(mpris_, Application::supported_uri_schemes().data());
    org_mpris_media_player2_set_supported_mime_types(mpris_, Application::supported_mime_types().data());

    org_mpris_media_player2_player_set_playback_status(player_, "Stopped");
    org_mpris_media_player2_player_set_loop_status(player_, "None");
    org_mpris_media_player2_player_set_rate(player_, 1.0);
    org_mpris_media_player2_player_set_shuffle(player_, false);
    g_variant_builder_init(&bld, G_VARIANT_TYPE("a{sv}"));
    value = g_variant_builder_end(&bld);
    org_mpris_media_player2_player_set_metadata(player_, value);
    org_mpris_media_player2_player_set_volume(player_, 1.0);
    org_mpris_media_player2_player_set_position(player_, 0);
    org_mpris_media_player2_player_set_minimum_rate(player_, Player_State::min_speed / 100.0);
    org_mpris_media_player2_player_set_maximum_rate(player_, Player_State::max_speed / 100.0);
    org_mpris_media_player2_player_set_can_go_next(player_, true);
    org_mpris_media_player2_player_set_can_go_previous(player_, true);
    org_mpris_media_player2_player_set_can_play(player_, true);
    org_mpris_media_player2_player_set_can_pause(player_, true);
    org_mpris_media_player2_player_set_can_seek(player_, true);
    org_mpris_media_player2_player_set_can_control(player_, true);

    //install_property_change_callback(mpris_, "fullscreen", &Impl::fullscreen_changed);
    install_property_change_callback(player_, "loop-status", &Impl::loop_status_changed);
    install_property_change_callback(player_, "rate", &Impl::rate_changed);
    //install_property_change_callback(player_, "shuffle", &Impl::shuffle_changed);
    install_property_change_callback(player_, "volume", &Impl::volume_changed);
}

void Mpris_Server::Impl::init_methods()
{
    OrgMprisMediaPlayer2Iface *mpris_if = ORG_MPRIS_MEDIA_PLAYER2_GET_IFACE(mpris_);
    OrgMprisMediaPlayer2PlayerIface *player_if = ORG_MPRIS_MEDIA_PLAYER2_PLAYER_GET_IFACE(player_);

    mpris_if->handle_quit = &Impl::handle_quit;
    mpris_if->handle_raise = &Impl::handle_raise;

    player_if->handle_next = &Impl::handle_next;
    player_if->handle_open_uri = &Impl::handle_open_uri;
    player_if->handle_pause = &Impl::handle_pause;
    player_if->handle_play = &Impl::handle_play;
    player_if->handle_play_pause = &Impl::handle_play_pause;
    player_if->handle_previous = &Impl::handle_previous;
    player_if->handle_seek = &Impl::handle_seek;
    player_if->handle_set_position = &Impl::handle_set_position;
    player_if->handle_stop = &Impl::handle_stop;
}

void Mpris_Server::Impl::install_property_change_callback(gpointer object, const gchar *prop_name, void (Impl:: *callback)(GValue *))
{
    std::string signal = std::string("notify::") + prop_name;

    PropertySlot *pslot = new PropertySlot;
    property_slots_.emplace_back(pslot);

    pslot->self = this;
    pslot->object = object;
    pslot->callback = callback;
    pslot->connection = g_signal_connect(
        object, signal.c_str(), (GCallback)+[](GObject *gobject, GParamSpec *pspec, gpointer user_data)
    {
        PropertySlot &pslot = *(PropertySlot *)user_data;

        GValue value {};
        g_value_init(&value, pspec->value_type);
        auto value_cleanup = nonstd::make_scope_exit([&value]() { g_value_unset(&value); });
        g_object_get_property(gobject, pspec->name, &value);

        (pslot.self->*pslot.callback)(&value);
    }, pslot);
}

void Mpris_Server::Impl::enable_property_change_callbacks(bool enable)
{
    size_t count = property_slots_.size();
    for (size_t i = 0; i < count; ++i) {
        PropertySlot &pslot = *property_slots_[i];
        if (enable)
            g_signal_handler_unblock(pslot.object, pslot.connection);
        else
            g_signal_handler_block(pslot.object, pslot.connection);
    }
}

void Mpris_Server::Impl::on_bus_acquired(GDBusConnection *connection, const gchar *name, gpointer user_data)
{
    Log::i("MPRIS bus acquired: %s", name);

    Impl &impl = *(Impl *)user_data;

    const std::pair<const char *, GDBusInterfaceSkeleton *> interfaces[] = {
        {"/org/mpris/MediaPlayer2", G_DBUS_INTERFACE_SKELETON(impl.mpris_)},
        {"/org/mpris/MediaPlayer2", G_DBUS_INTERFACE_SKELETON(impl.player_)},
    };

    for (std::pair<const char *, GDBusInterfaceSkeleton *> item : interfaces) {
        GDBusInterfaceInfo *info = g_dbus_interface_skeleton_get_info(item.second);
        if (!g_dbus_interface_skeleton_export(item.second, connection, item.first, nullptr))
            Log::e("MPRIS interface failed to register: %s", info->name);
        else
            Log::i("MPRIS interface registered: %s", info->name);
    }
}

void Mpris_Server::Impl::on_name_acquired(GDBusConnection *connection, const gchar *name, gpointer user_data)
{
    Log::i("MPRIS name acquired: %s", name);
}

void Mpris_Server::Impl::on_name_lost(GDBusConnection *connection, const gchar *name, gpointer user_data)
{
    Log::i("MPRIS name lost: %s", name);

    Impl &impl = *(Impl *)user_data;

    impl.connection_over_ = true;
}

//------------------------------------------------------------------------------
// helpers

const std::string &Mpris_Server::Impl::get_track_id_prefix()
{
    static const std::string prefix = "/org/sdf1/jpcima/smf_dsp/instance" + std::to_string(getpid()) + "/track";
    return prefix;
}

std::string Mpris_Server::Impl::make_track_id(uint64_t serial)
{
    const std::string &prefix = get_track_id_prefix();
    return prefix + std::to_string(serial);
}

bool Mpris_Server::Impl::parse_track_id(const gchar *id, uint64_t &serial)
{
    const std::string &prefix = get_track_id_prefix();
    nonstd::string_view src{id};

    if (!src.starts_with(prefix))
        return false;
    src.remove_prefix(prefix.size());

    unsigned count = 0;
    if (sscanf(src.data(), "%" SCNu64 "%n", &serial, &count) != 1 || count != src.size())
        return false;

    return true;
}

//------------------------------------------------------------------------------
// org.mpris.MediaPlayer2

gboolean Mpris_Server::Impl::handle_quit (
    OrgMprisMediaPlayer2 *object,
    GDBusMethodInvocation *invocation)
{
    if (mpris_verbose)
        Log::i("MPRIS: Quit()");

    Impl &impl = *(Impl *)g_object_get_data(G_OBJECT(object), "my:self");
    impl.app_->engage_shutdown();

    g_dbus_method_invocation_return_value(invocation, nullptr);
    return true;
}

gboolean Mpris_Server::Impl::handle_raise (
    OrgMprisMediaPlayer2 *object,
    GDBusMethodInvocation *invocation)
{
    if (mpris_verbose)
        Log::i("MPRIS: Raise()");

    Impl &impl = *(Impl *)g_object_get_data(G_OBJECT(object), "my:self");
    impl.app_->raise_window();

    g_dbus_method_invocation_return_value(invocation, nullptr);
    return true;
}

//------------------------------------------------------------------------------
// org.mpris.MediaPlayer2.Player

gboolean Mpris_Server::Impl::handle_next (
    OrgMprisMediaPlayer2Player *object,
    GDBusMethodInvocation *invocation)
{
    if (mpris_verbose)
        Log::i("MPRIS: Next()");

    Impl &impl = *(Impl *)g_object_get_data(G_OBJECT(object), "my:self");
    impl.app_->advance_playlist_by(+1);

    g_dbus_method_invocation_return_value(invocation, nullptr);
    return true;
}

gboolean Mpris_Server::Impl::handle_open_uri (
    OrgMprisMediaPlayer2Player *object,
    GDBusMethodInvocation *invocation,
    const gchar *arg_Uri)
{
    if (mpris_verbose)
        Log::i("MPRIS: OpenUri(\"%s\")", arg_Uri);

    Impl &impl = *(Impl *)g_object_get_data(G_OBJECT(object), "my:self");

    Uri_Split us;
    if (parse_uri(arg_Uri, us)) {
        if ((us.scheme.empty() || us.scheme == "file") && !us.path.empty())
            impl.app_->play_full_path(us.path);
    }

    g_dbus_method_invocation_return_value(invocation, nullptr);
    return true;
}

gboolean Mpris_Server::Impl::handle_pause (
    OrgMprisMediaPlayer2Player *object,
    GDBusMethodInvocation *invocation)
{
    if (mpris_verbose)
        Log::i("MPRIS: Pause()");

    Impl &impl = *(Impl *)g_object_get_data(G_OBJECT(object), "my:self");
    impl.app_->pause_playback();

    g_dbus_method_invocation_return_value(invocation, nullptr);
    return true;
}

gboolean Mpris_Server::Impl::handle_play (
    OrgMprisMediaPlayer2Player *object,
    GDBusMethodInvocation *invocation)
{
    if (mpris_verbose)
        Log::i("MPRIS: Play()");

    Impl &impl = *(Impl *)g_object_get_data(G_OBJECT(object), "my:self");
    impl.app_->resume_playback();

    g_dbus_method_invocation_return_value(invocation, nullptr);
    return true;
}

gboolean Mpris_Server::Impl::handle_play_pause (
    OrgMprisMediaPlayer2Player *object,
    GDBusMethodInvocation *invocation)
{
    if (mpris_verbose)
        Log::i("MPRIS: PlayPause()");

    Impl &impl = *(Impl *)g_object_get_data(G_OBJECT(object), "my:self");
    impl.app_->toggle_pause_playback();

    g_dbus_method_invocation_return_value(invocation, nullptr);
    return true;
}

gboolean Mpris_Server::Impl::handle_previous (
    OrgMprisMediaPlayer2Player *object,
    GDBusMethodInvocation *invocation)
{
    if (mpris_verbose)
        Log::i("MPRIS: Previous()");

    Impl &impl = *(Impl *)g_object_get_data(G_OBJECT(object), "my:self");
    impl.app_->advance_playlist_by(-1);

    g_dbus_method_invocation_return_value(invocation, nullptr);
    return true;
}

gboolean Mpris_Server::Impl::handle_seek (
    OrgMprisMediaPlayer2Player *object,
    GDBusMethodInvocation *invocation,
    gint64 arg_Offset)
{
    if (mpris_verbose)
        Log::i("MPRIS: Seek(%" PRId64 ")", arg_Offset);

    Impl &impl = *(Impl *)g_object_get_data(G_OBJECT(object), "my:self");
    impl.app_->seek_by(arg_Offset * 1e-6);

    g_dbus_method_invocation_return_value(invocation, nullptr);
    return true;
}

gboolean Mpris_Server::Impl::handle_set_position (
    OrgMprisMediaPlayer2Player *object,
    GDBusMethodInvocation *invocation,
    const gchar *arg_TrackId,
    gint64 arg_Position)
{
    if (mpris_verbose)
        Log::i("MPRIS: SetPosition(\"%s\", %" PRId64 ")", arg_TrackId, arg_Position);

    Impl &impl = *(Impl *)g_object_get_data(G_OBJECT(object), "my:self");

    uint64_t serial = 0;
    if (!parse_track_id(arg_TrackId, serial)) {
        if (mpris_verbose)
            Log::w("Could not parse track ID `%s`", arg_TrackId);
    }
    else {
        if (impl.last_state_.song_serial != serial) {
            if (mpris_verbose)
                Log::w("Track ID is not matching: got %" PRIu64 ", expected %" PRIu64,
                       serial, impl.last_state_.song_serial);
        }
        else
            impl.app_->seek_to(arg_Position * 1e-6);
    }

    g_dbus_method_invocation_return_value(invocation, nullptr);
    return true;
}

gboolean Mpris_Server::Impl::handle_stop (
    OrgMprisMediaPlayer2Player *object,
    GDBusMethodInvocation *invocation)
{
    if (mpris_verbose)
        Log::i("MPRIS: Stop()");

    Impl &impl = *(Impl *)g_object_get_data(G_OBJECT(object), "my:self");
    impl.app_->stop_playback();

    g_dbus_method_invocation_return_value(invocation, nullptr);
    return true;
}

void Mpris_Server::Impl::loop_status_changed(GValue *value)
{
    nonstd::string_view loop_status = g_value_get_string(value);

    if (mpris_verbose)
        Log::i("MPRIS: LoopStatus(\"%s\")", loop_status.data());

    unsigned repeat_mode;
    if (loop_status == "None")
        repeat_mode = Repeat_Single|Repeat_Off;
    else if (loop_status == "Track")
        repeat_mode = Repeat_Single|Repeat_On;
    else if (loop_status == "Playlist")
        repeat_mode = Repeat_Multi|Repeat_On;
    else
        return;

    app_->set_repeat_mode(repeat_mode);
}

void Mpris_Server::Impl::rate_changed(GValue *value)
{
    double rate = g_value_get_double(value);

    if (mpris_verbose)
        Log::i("MPRIS: Rate(%f)", rate);

    if (rate <= 0)
        app_->pause_playback();
    else {
        int speed = (int)(0.5 + rate * 100);
        app_->set_playback_speed(speed, false);
    }
}

void Mpris_Server::Impl::volume_changed(GValue *value)
{
    double volume = g_value_get_double(value);

    if (mpris_verbose)
        Log::i("MPRIS: Volume(%f)", volume);

    app_->set_playback_volume(volume);
}

#endif // defined(HAVE_MPRIS)
