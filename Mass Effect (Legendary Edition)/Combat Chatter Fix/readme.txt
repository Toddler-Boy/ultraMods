Combat Chatter Fix for Mass Effect (Legendary Edition)

The problem
In Mass Effect 1 the short combat voice lines of enemies and squad mates fall silent after loading a save or
travelling to another world: the Geth's electronic death cries and pain noises, the attack and pain grunts of human
and krogan enemies, the squad's remarks in a fight. They come back only after a while, which is why the Geth seem mute
early in the game and only now and then later.

The cause
All combat voice lines go through one cooldown table that every character shares. For each kind of line (death cry,
pain grunt, attack grunt, ...) it remembers the game clock at which that kind last played, and a line may play only
once enough time has passed since then. The game clock restarts at zero on every load, but the table keeps its time
stamps from before the load. Until the new clock catches up with the old stamp, that kind of line stays silent: a
death cry that last played 40 minutes into the previous map silences every death cry for the first 40 minutes after
the load.

The fix
One condition is added to the check: a time stamp later than the current clock comes from before the load and counts
as expired. Every designed cooldown within one session in a map stays as it was (90 seconds between attack grunts,
10 seconds between pain grunts, and so on). The changed function, BioChatterSystem.CanPlayLine in SFXGame.pcc, is the
game's own with this one condition added; nothing else changes.

Install
A merge mod: install it with ME3Tweaks Mod Manager (drag the zip onto it, then Apply). It merges
into SFXGame.pcc and works together with other mods that merge into that file, the LE1 Community Patch included.

Uninstall
Remove it in ME3Tweaks Mod Manager and let the merges be rebuilt.

Notes
- Works for every enemy and squad mate with combat voice lines, in every level.
- Source: github.com/Toddler-Boy/ultraMods
