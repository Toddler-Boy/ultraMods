#!/bin/bash -e
# Builds every mod, zips each one for Nexus (Builds/vs/mods/<game>/<mod>/<mod>.zip) and copies the built files into
# ultraGame's curation on the share
cd "$(dirname "$0")"
BUILD_DIR="$(cygpath -m "$PWD")/Builds/vs"
mkdir -p Builds

# Configure every time: a new mod folder is found by a glob the generated project does not watch
cmake --preset vs > Builds/configure.log 2>&1 || { cat Builds/configure.log; exit 1; }
cmake --build --preset vs --parallel 2>&1 | tee Builds/build.log

if grep -qiE 'warning|error' Builds/build.log; then
    read -p "Warnings in Builds/build.log, press enter to close"
fi

DEST="Z:/Data/ultraGame/curation"
for mod in "$BUILD_DIR"/mods/*/*/; do
    mod="${mod%/}"
    name="$(basename "$mod")"
    game="$(basename "$(dirname "$mod")")"
    to="$(cat "$mod/deploy.txt")"

    (cd "$mod" && rm -f "$name.zip" && 7z a -tzip -bso0 -bsp0 "$name.zip" ./*.dll readme.txt LICENSE-MinHook.txt)

    # -p keeps the build's time stamp: a fresh one on every deploy would mark every install outdated
    mkdir -p "$DEST/$game/mods/$name/$to"
    cp -p "$mod"/*.dll "$DEST/$game/mods/$name/$to/"
done
