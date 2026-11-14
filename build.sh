#!/bin/bash -e
# Builds every mod and zips each one for Nexus Mods (Builds/vs/mods/<game>/<mod>/<mod>.zip)
cd "$(dirname "$0")"
BUILD_DIR="$(cygpath -m "$PWD")/Builds/vs"
mkdir -p Builds

# Configure every time: a new mod folder is found by a glob the generated project does not watch
cmake --preset vs > Builds/configure.log 2>&1 || { cat Builds/configure.log; exit 1; }
cmake --build --preset vs --parallel 2>&1 | tee Builds/build.log

if grep -qiE 'warning|error' Builds/build.log; then
    read -p "Warnings in Builds/build.log, press enter to close"
fi

for mod in "$BUILD_DIR"/mods/*/*/; do
    mod="${mod%/}"
    name="$(basename "$mod")"

    # A merge mod: Mod Manager's layout (package/) at the zip's root
    if [ -d "$mod/package" ]; then
        (cd "$mod" && rm -f "$name.zip" && 7z a -tzip -bso0 -bsp0 "$name.zip" ./package/* readme.txt LICENSE.txt)
    else
        (cd "$mod" && rm -f "$name.zip" && 7z a -tzip -bso0 -bsp0 "$name.zip" ./*.dll readme.txt LICENSE-MinHook.txt)
    fi
done
