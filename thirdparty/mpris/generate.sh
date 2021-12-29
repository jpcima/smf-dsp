#!/bin/bash
set -e

interfaces=("org.mpris.MediaPlayer2"
            "org.mpris.MediaPlayer2.Player"
            "org.mpris.MediaPlayer2.Playlists"
            "org.mpris.MediaPlayer2.TrackList")

cd src
for interface in "${interfaces[@]}"; do
    gdbus-codegen --generate-c-code "$interface" "../spec/$interface.xml"
done
cd ..
