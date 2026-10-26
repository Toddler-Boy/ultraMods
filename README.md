# ultraMods

Game mods, one folder per game and one per mod inside it. Code that more than one game shares sits in `common`.

| Game | Mod | What it does |
|---|---|---|
| Watch_Dogs | Spatial Audio Fix | Sound with Windows spatial audio (Dolby Atmos, DTS:X) on an HDMI receiver: an `xinput1_3.dll` stand-in that reports the output device to XAudio2 2.7 as PCM |
| Far Cry 4 | Spatial Audio Fix | The same, as `xinput9_1_0.dll` |
| Far Cry Primal | Spatial Audio Fix | The same, as `xinput9_1_0.dll` |

`ultraPatcher` switches a game's byte patches on and off, see its [README](ultraPatcher/README.md).

## Building

Windows, Visual Studio 2026 with clang-cl, CMake 4.3, 7-Zip on the path.

```
./build.sh
```

Every mod ends up in `Builds/vs/mods/<game>/<mod>/` with its readme and licences, zipped as `<mod>.zip` ready for
Nexus. The script also copies the built files into ultraGame's curation on the share.
