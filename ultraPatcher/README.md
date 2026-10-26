# ultraPatcher

A small Windows tool that switches a game's byte patches on and off. Each game gets its own `ultraPatcher.exe` with that
game's patches inside. Put it next to the game's exe and start it: one checkbox per patch, a slider for patches that
take a value. Every change is written to the game's file at once, unticking writes the original bytes back.

Before it shows anything, ultraPatcher checks every byte of every patch in the game's files. Each patch must read either
fully original or fully patched. If even one doesn't, the game is a different version than the patches were made for:
ultraPatcher says so and closes without touching anything. Before each write it checks those bytes again.

It needs administrator rights because games usually sit under Program Files.

## What's inside

`ultraPatcher.cpp` is the whole program: plain Win32, no libraries. The patches are JSON appended to the end of the
exe, in the format ultraGame's `game.json` uses:

```
{ "title": "...", "exe": "Game.exe", "patches": [
  { "what": "Rain at {}%", "value": { "min", "max", "default", "step", "unit" },
    "blocks": [ { "file": "Game.exe", "offset": 123456, "original": "F3440F105108", "patched": "E978176A0090",
                  "slot": { "at", "type", "from", "to" } } ] } ] }
```

After the JSON come its size as a 32-bit number and `ULTRAPAT`. To read them yourself, open the exe in a text editor
and look at the end, or before the signature at the very end if the exe is signed.

## Building

`./build.sh` in the repository root builds the generic `Builds/vs/ultraPatcher/Release/ultraPatcher.exe`, which has
no patches. A game's patcher is a copy of it with that game's patches appended:

```
python ultraPatcher/make_patcher.py "<game.json>" Builds/vs/ultraPatcher/Release/ultraPatcher.exe <out.exe>
```

Sign `<out.exe>` afterwards, never the generic exe before appending: the signature then covers the patches too.
