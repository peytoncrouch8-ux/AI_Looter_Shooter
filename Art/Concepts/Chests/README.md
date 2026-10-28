# Loot chests: Tripo views (2026-10-27)

Five chest tiers, 1 = worst loot, 5 = best. Source: `Art/Models/Openables/Loot+chests.glb` (a Tripo model of a concept
sheet: one relief, one row per tier; per row closed front, closed side, open lid + base). The user models each chest in
Tripo, one at a time, the same way as the AR gun parts (`Art/Concepts/Weapons/AR/README.md`).

| Tier | Look | Front features left off the back |
|---|---|---|
| Tier1 | olive drab case, two strap latches, green latch light | green latch window |
| Tier2 | orange hard case, grey straps, wire side handles, yellow light bar | light bar and its lid tab |
| Tier3 | grey case, black straps, blue ring lock | ring lock and its blue lines (base trim kept) |
| Tier4 | dark purple carved stone, gold trim, violet orb | orb and the violet line under it (lid seam glow kept) |
| Tier5 | red, silver and gold crowned chest, glowing gem | gold emblem, gem and glow lines (crown kept) |

## Files per tier (`Tier<N>/`)

- `upload/front.png, left.png, back.png, right.png`: the CLOSED chest alone, 1024 x 1024, transparent, centred,
  widest view fills 80%, same scale and same height in all four. Upload these.
- `upload_white/`: the same on pure white (use if Tripo's transparent upload misbehaves).
- `open_ref.png`: open lid and base from the sheet. Reference only, never upload.

How they were made: orthographic straight-on Blender render of the sheet, flat light (Workbench, texture colour). Each
view cut out, edges smoothed and pulled in a few px, specks and stray pieces dropped, small holes filled. Side views
scaled to the front's height (the sheet drew sides ~8% bigger). Left = right mirrored (the sheet has one side). Back =
front mirrored with the front-only features painted over from the panels beside them; straps and latches stay as hinges.
Scripts: `make/render_sheet.py`, `make/build_views.py`, boxes in `make/cfg.json` (run lines in the script headers).

Notes:
- Tier5: a thin ring floated above the closed chest on the sheet, not attached; dropped. The sheet's tier 5 side view
  (pointed crest) doesn't match its front (flared box, crown dipping in the middle); the side is scaled to the front's
  full height, so Tripo has to reconcile them. Check that chest's shape first.
- Tier2: the wire side handles stick out past the body in front/back; they are the side view's handle seen edge-on.
- The sheet's texture is painterly with some smears (Tier4 around the orb); Tripo's Texture step repaints anyway.

## Tripo steps (per tier)

1. Multiview image to 3D, one chest at a time: front, left, back, right = `Tier<N>/upload/front.png, left.png,
   back.png, right.png`.
2. Model the chest CLOSED, in one piece. Don't try to split the lid.
3. Smart UV, then Texture (PBR if offered).
4. Export GLB and save it in `Tier<N>/upload/` (`Tier<N>.glb`).

Main then splits the lid from the base in Blender (hinge at the back), hollows the inside, adds the pivot, collision and
sockets, and imports it like any other model (`Art/README.md`).
