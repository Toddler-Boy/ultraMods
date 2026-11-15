Timed Autosave for Mass Effect (Legendary Edition)

The problem
Mass Effect 1 autosaves only where a mission asks for it, mostly at a mission's start and end. Walking around the
Citadel, shopping, talking and doing side missions can go on for hours without a single autosave. A crash or a death
then costs everything since the last save made by hand.

What the mod does
Every 5 minutes it lets the game make its regular autosave, the same one the missions trigger, into the same autosave
slot.

- The game's own rules decide whether saving is allowed at that moment, with the same check the menu's Save entry and
  the quick save key use: no autosave in combat, in conversations, cutscenes and minigames, in a no-save area, while
  dead or with the Mako in a bad spot. None while a full-screen menu is open either.
- When saving is not allowed at the 5 minute mark, that autosave is skipped and the next one is due 5 minutes later.
  Nothing keeps trying in the background.
- The 5 minutes count from the last save of any kind: a manual save, a quick save, one of the game's own autosaves,
  and also from loading a save. Save by hand often enough and the mod never has to do anything.
- The game's Autosave option switches it off together with the game's own autosaves.

How it works
The game ships with this timer built in and switched off. BioSaveGame.TryAutoSaving runs every frame; it would
autosave every m_fAutoSaveTime seconds (300 in the game's settings) after checking that saving is allowed, but returns
at once because the setting m_bDissableAutoSave is on. The mod changes the one function that calls it,
BioWorldInfo.Tick in SFXGame.pcc: it switches the timer on, makes a refused attempt wait the full 5 minutes instead of
2 seconds, and hands the timer the time of the last save, which the engine keeps for every kind of save. The check and
the save itself are the game's unchanged code.

Install
A merge mod: install it with ME3Tweaks Mod Manager (drag the zip onto it, then Apply). It merges
into SFXGame.pcc and works together with other mods that merge into that file, the LE1 Community Patch included.

Uninstall
Remove it in ME3Tweaks Mod Manager and let the merges be rebuilt.

Notes
- The autosave slot holds one save per career. A timed autosave replaces the autosave before it, as every autosave of
  the game does. Manual saves and the quick save are never touched.
- The game shows its short saving notice, as with its own autosaves.
- The 5 minutes are real time: time spent in a menu or with the game paused counts too.
- Source: github.com/Toddler-Boy/ultraMods
