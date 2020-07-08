#!/bin/bash

# ---------------------------------------------------------------------------------------------------------------------
# stop on error

set -e

# ---------------------------------------------------------------------------------------------------------------------
# print shell commands

set -x

# ---------------------------------------------------------------------------------------------------------------------
# set variables

test ! -z "$_HOST"

# ---------------------------------------------------------------------------------------------------------------------

cat >>/etc/pacman.conf <<EOF
[mingw-w64]
SigLevel = Optional TrustAll
Server = https://github.com/jpcima/arch-mingw-w64/releases/download/repo.\$arch/
EOF

yes | pacman-key --refresh-keys
pacman -Sqy --noconfirm
pacman -Sq --noconfirm base-devel mingw-w64-gcc mingw-w64-winpthreads mingw-w64-cmake mingw-w64-pkg-config mingw-w64-win-iconv mingw-w64-icu mingw-w64-libuv mingw-w64-sdl2 dll-bundler-git

# ---------------------------------------------------------------------------------------------------------------------
# build the program

mkdir -p build
pushd build
"$_HOST"-cmake -DCMAKE_BUILD_TYPE=Release ..
make -j"$(nproc)"
for f in *.exe *.dll; do
    dll-bundler -L /usr/"$_HOST"/bin "$f"
done
popd
