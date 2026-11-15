# ultraMods

Game mods, one folder per game and one per mod inside it. Code that more than one game shares sits in `common`.

| Game | Mod | What it does |
|---|---|---|
| Watch_Dogs | Spatial Audio Fix | Sound with Windows spatial audio (Dolby Atmos, DTS:X) on an HDMI receiver: an `xinput1_3.dll` stand-in that reports the output device to XAudio2 2.7 as PCM |
| Far Cry 4 | Spatial Audio Fix | The same, as `xinput9_1_0.dll` |
| Far Cry Primal | Spatial Audio Fix | The same, as `xinput9_1_0.dll` |
| Mass Effect (Legendary Edition) | Combat Chatter Fix | Mass Effect 1's enemy and squad combat voice lines (the Geth's death cries among them) play again after loading a save or travelling: a merge mod adding one condition to the shared cooldown check, so time stamps from before the load count as expired |
| Mass Effect (Legendary Edition) | Timed Autosave | Mass Effect 1 autosaves every 5 minutes while the game allows saving, counted from the last save or load of any kind: a merge mod switching on the timed autosave the game ships switched off, through the one function that calls it |
| Mass Effect (Legendary Edition) | Cheat Keys | Mass Effect 1: Shift+F11 gives 50,000 credits and Shift+F10 gives 1,000 experience points through the game's own console commands: a DLC mod whose config delta adds the bindings to the game's key lists, the Coalesced file is not replaced |
| Baldur's Gate 3 | Always Roll 20 | Every d20 the party and its allies roll is a natural 20, damage dice roll their maximum, saves get +10 and initiative +20: a Script Extender mod giving them a status with the game's own roll boosts and a reaction that sets each attack roll to 20 |
| Baldur's Gate 3 | Unlucky Enemies | Every enemy attack on the party and its allies is a natural 1, and so are enemies' saving throws against their spells: a Script Extender mod giving them reactions that set the enemy's roll to 1 |

`ultraPatcher` switches a game's byte patches on and off, see its [README](ultraPatcher/README.md).

## Building

Windows, Visual Studio 2026 with clang-cl, CMake 4.3, 7-Zip on the path.

```
./build.sh
```

Every mod ends up in `Builds/vs/mods/<game>/<mod>/` with its readme and licences, zipped as `<mod>.zip` ready for
Nexus Mods. Mass Effect merge mods ship their checked-in `.m3m`; Baldur's Gate 3 mods are packed into a `.pak` with
Norbyte's LSLib, which the build downloads.
