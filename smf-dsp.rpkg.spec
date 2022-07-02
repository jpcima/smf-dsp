Name:       {{{ git_dir_name }}}

Version:    {{{ git_dir_version }}}

Release:    1%{?dist}

Summary:    Advanced MIDI file player, including various chip music synths

License:    GPLv2+

URL:        https://github.com/jpcima/smf-dsp

VCS:        {{{ git_dir_vcs }}}

Source0:    {{{ git_dir_pack }}}
Source1:    {{{ git_pack dir_name=libADLMIDI       path=thirdparty/libADLMIDI }}}
Source2:    {{{ git_pack dir_name=libOPNMIDI       path=thirdparty/libOPNMIDI }}}
Source3:    {{{ git_pack dir_name=scc              path=thirdparty/scc }}}
Source4:    {{{ git_pack dir_name=munt             path=thirdparty/munt }}}
Source5:    {{{ git_pack dir_name=fluidlite        path=thirdparty/fluidlite }}}
Source6:    {{{ git_pack dir_name=timidityplus     path=thirdparty/timidityplus }}}
Source7:    {{{ git_pack dir_name=span-lite        path=thirdparty/span-lite }}}
Source8:    {{{ git_pack dir_name=scope-lite       path=thirdparty/scope-lite }}}
Source9:    {{{ git_pack dir_name=string-view-lite path=thirdparty/string-view-lite }}}

BuildRequires: cmake
BuildRequires: gcc-c++
BuildRequires: SDL2-devel
BuildRequires: SDL2_image-devel
BuildRequires: libuv-devel
BuildRequires: glib2-devel
BuildRequires: (jack-audio-connection-kit-devel or pipewire-jack-audio-connection-kit-devel)
BuildRequires: alsa-lib-devel
BuildRequires: libsoundio-devel
BuildRequires: apr-devel
BuildRequires: apr-util-devel

%description
Advanced MIDI file player, including various chip music synths

%prep
{{{ git_dir_setup_macro }}}
tar -C thirdparty -xf %{SOURCE1}
tar -C thirdparty -xf %{SOURCE2}
tar -C thirdparty -xf %{SOURCE3}
tar -C thirdparty -xf %{SOURCE4}
tar -C thirdparty -xf %{SOURCE5}
tar -C thirdparty -xf %{SOURCE6}
tar -C thirdparty -xf %{SOURCE7}
tar -C thirdparty -xf %{SOURCE8}
tar -C thirdparty -xf %{SOURCE9}

%build
%cmake -DSMF_DSP_PLUGIN_DIR=%{_libdir}/smf-dsp -DSMF_DSP_DEFAULT_SF2=%{_datadir}/soundfonts/default.sf2
%cmake_build

%install
%cmake_install

%files
%{_bindir}/smf-dsp
%{_libdir}/smf-dsp/*.so
%{_datadir}/applications/smf-dsp.desktop
%{_datadir}/icons/hicolor/16x16/apps/smf-dsp.png
%{_datadir}/icons/hicolor/64x64/apps/smf-dsp.png
%{_datadir}/metainfo/io.github.jpcima.smf_dsp.metainfo.xml

%changelog
{{{ git_dir_changelog }}}
