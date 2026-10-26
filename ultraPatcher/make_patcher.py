"""Appends a game's patches from its ultraGame game.json to a copy of the generic ultraPatcher.exe. Sign the result
afterwards: the signature then covers the patches too.
usage: make_patcher.py <game.json> <ultraPatcher.exe> <out.exe>"""
import json
import struct
import sys


def blocks(patch):
    for b in patch.get("blocks") or [patch]:
        original = b["original"].replace(" ", "").upper()
        patched = b["patched"].replace(" ", "").upper()
        if len(original) != len(patched) or len(original) % 2:
            sys.exit(f"{patch['what']}: original and patched bytes differ in length")

        out = {"file": b.get("file", patch["file"]), "offset": int(b["offset"], 0), "original": original, "patched": patched}
        if "slot" in b:
            out["slot"] = b["slot"]
        yield out


def main(game_json, generic_exe, out_exe):
    with open(game_json, encoding="utf-8") as f:
        game = json.load(f)
    with open(generic_exe, "rb") as f:
        exe = f.read()

    pe = struct.unpack_from("<I", exe, 0x3C)[0]
    if struct.unpack_from("<I", exe, pe + 24 + 144)[0]:
        sys.exit(f"{generic_exe} is signed, append to the unsigned build and sign afterwards")

    patches = [{"what": p["what"], **({"value": p["value"]} if "value" in p else {}), "blocks": list(blocks(p))}
               for p in sorted(game.get("patches", []), key=lambda p: p["what"].casefold())]
    if not patches:
        sys.exit(f"{game_json} has no patches")

    data = json.dumps({"title": game["title"], "exe": game["exe"].split("/")[-1], "patches": patches},
                      ensure_ascii=False, separators=(",", ":")).encode("utf-8")

    padding = b"\0" * (-(len(exe) + len(data) + 12) % 8)

    with open(out_exe, "wb") as f:
        f.write(exe + padding + data + struct.pack("<I", len(data)) + b"ULTRAPAT")

    print(f"{out_exe}: {len(patches)} patches for {game['title']}")


if __name__ == "__main__":
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    main(*sys.argv[1:])
