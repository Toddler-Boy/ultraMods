Spatial Audio Fix for @GAME@

The problem
With Windows spatial sound switched on for an HDMI receiver (Dolby Atmos for home theater, DTS:X for home theater),
@GAME@ plays the intro videos with sound and then goes silent for good once the game itself starts playing audio.
Turning spatial sound off, or switching the output to stereo, brings the sound back.

The cause
The game's sound engine asks XAudio2 for the output device's format and refuses to start unless that format is plain
PCM. With spatial sound on, Windows describes the device as a Dolby MAT bitstream (8 channels at 192 kHz), so the
engine gives up and nothing is heard. The intro videos use a separate XAudio2 instance that does not check, which is
why they still play.

The fix
@DLL@ is a small stand-in for the system DLL of the same name. The game loads it instead of the system one, the
XInput calls are passed straight through, and the one XAudio2 call that reports the device format answers PCM when it
would otherwise report a bitstream format. Nothing else changes: the game mixes 5.1 as it always did, Windows turns
that into Atmos as it does for any other game.

Install
Copy @DLL@ into the game's @TO@ folder, beside @EXE@:
  ...\@GAME@\@TO@\@DLL@

Uninstall
Delete that file again.

Notes
- The file spatial-audio-fix.log appears beside @EXE@ on every start and says what device format the game was shown.
  If something does not work, that file is what to post.
- Another mod that ships its own @DLL@ in the @TO@ folder cannot be used at the same time.
- Built with MinHook (BSD 2-clause licence, see LICENSE-MinHook.txt). Source: github.com/Toddler-Boy/ultraMods
