# Grip spots on the parts (2026-10-29)

The user: "Are we doing this the wrong way? ... is there totally a different way to code/implement the hands in first person
instead of having to adjust them individually per gun?" Picked: **grip spots on the parts** (markers placed in Blender on
each held part; a handful of hand poses made once; the game reaches the arms to the markers). The AR's support hand: cupped
under the trigger housing (the thumbhole loop's front), fingers closed onto it, visible in first person (the user, same day).

## Marker grammar (static mesh sockets from Blender empties; rotation survives: looter_export.socket_entry -> ModelImporter.cpp:493)
- `SOCKET_<Role>_<L|R>_<Pose>`: Role `Hold` (the resting hold), `Rack` (pump/bolt work), `Reload` (where the support hand
  takes the reload part). Side L = support (left), R = trigger (right). Pose = a name from the pose library.
- `SOCKET_Trigger`: location only (the trigger's face), replaces `FGunHands::TriggerAt` when present.
- Meaning: the marker sits on the part's skin under the palm's grip point. Socket +X = the knuckles' way (fingers point),
  socket +Z = the palm's facing (toward the part, i.e. out of the part's surface toward the palm, reversed: the palm looks
  along -Z into the part). Same frame as `FHandPlacement` (X fingers, Y palm via `FirstPersonHands::Frame`): lane C converts.
- Gun space: the part's modeled cm (the game scales with the gun's drawn size). Palm-skin offsets are hand-sized: added in
  game at the arms' scale, never baked into the marker.
- Priority: within one Role+Side the latest part in assembly order wins (as `UWeaponModelComponent::FindSocket`). Roles never
  compete. A part wearing another's mesh (CSV Model column) gets its markers for free.
- Tripo parts: `looter_tripo` copies the scripted part's sockets; a Tripo spec's own sockets gain an optional rotation:
  `{name: ((u, v), w)}` or `{name: ((u, v), w, (rx, ry, rz))}`.

## Pose library (lane B): `Art/Models/Characters/FPHandPoses.py` -> generated `Source/AI_Looter_Shooter/Player/FirstPersonHandPoseData.inl` + `Saved/FPHands/poses.json`
- Poses: `Pistol`, `Cup`, `Pump`, `Forend`, `Wrap` (reload grab), `Revolver` (two-hand support on a revolver).
- Each: per finger (thumb, index, middle, ring, pinky) three joint curls in degrees from straight and a spread, thumb swing
  and spread (FHandFingers units), `TouchShare` (0..1, how much touch-closing may still bend the fingers), `HandleHalfAcross`,
  `HandleHalfThrough`, `HandleCorner` (cm, the default handle the touch-closing closes on), `bForearmBehindKnuckles` (the
  cup's rule: forearm straight back under the palm).
- Generated C++: `namespace FirstPersonHandPoses { struct FHandPose {...}; const FHandPose* Find(FName Pose); }` in
  `Player/FirstPersonHandPoses.h/.cpp` (lane B writes these two and the .inl; lane C only calls `Find`).
- Start from today's numbers (`FHandFingers::Of(EHandGrip)`, the `Make*` tables in FirstPersonHandsGrips.cpp).

## Game side (lane C)
- `UWeaponModelComponent::GetHandMarks(FHandMarks& Out) const` (WeaponModelComponent.h/.cpp, or a new
  WeaponModelHandMarks.cpp): parses every part's sockets; `FHandMark { bool bValid; FTransform Local (gun modeled cm,
  component space, live via Part->GetSocketTransform(RTS_Component) / component scale); FName Pose; }`;
  `FHandMarks { FHandMark Hold[2], Rack[2], Reload[2]; bool bTrigger; FVector Trigger; }` (index 0 = L, 1 = R, as
  `FirstPersonHands::Index`). Struct in `Weapons/WeaponHandMarks.h`.
- `FHandsGun` gains `FHandMarks Marks` and `float HandScale`; `DescribeGun` fills them.
- New `Player/FirstPersonHandsMarks.cpp/.h`: `FirstPersonHandsMarks::Placement(const FHandsGun&, const FHandMark&,
  EFirstPersonHand) -> FHandPlacement` (turn from the marker, hold = the marker point + palm direction x GripOffPalmSkin x
  HandScale, fingers from the pose, forearm zero so FrameForearm sets it unless the pose says behind-knuckles; contact
  handle from the marker frame + the pose's handle size, touch share capped by the pose).
- Seams: `OnGrip` / `OnForegrip` / `OnCup` use `Gun.Marks` first, else today's tables (fallback until every part is
  marked); `NextPumpShare` uses Rack_L; `SupportInReload` uses Reload_L when present; the component's cupped test asks the
  marker's pose. `Looter.Hands.Marks 0` forces the tables (before/after).
- Stays: Rig, Framing (FrameForearm, hip/framing nudges, revolver parking), reload/throw/cast/swim/bolt motions.
- Later (after every gun is marked): delete GripShapes, Cup, CupFit; shrink FGunHands.
- Tests: new `Looter.Player.Hands.Marks` (a test gun with marker sockets: parse, palm faces the part, within reach,
  fallback without markers). Keep every existing Player.Hands / FPHands test passing.

## Blender tools (lane A)
- `Tools/Blender/looter_guns.py`: a marker helper `g.hand_mark(model, role, side, pose, uv, w, fingers_dir, palm_dir)`
  (two directions in gun space u, v, w -> the empty's rotation so the exported socket has +X = fingers, +Z = palm facing).
- Bullpup.py's Tripo socket tuple gains the optional rotation (above).
- `fp_arms_sheet.py` `load_gun` keeps the sockets' full matrices.
- New `Tools/Blender/fp_hands_parts_sheet.py`: for each gun and each value of each held slot, assemble the gun, pose the
  arms from its markers (pose json + touch-closing), drawn at the game's sizes (gun x ScaleFor x 1.2 FP, arms 0.88, the
  hip hold of WeaponHipHold.cpp, 90 deg 16:9 eye view + one side view), tiles to `Saved/ArtPreviews/Weapons/Hands/<Gun>_<Slot>_<Key>.png`
  and a sheet per gun; honour LOOTER_BULLPUP_TRIPO; print palm gap / touched fingers / trigger reached per tile.

## Marking the guns (lanes D, after A): Bullpup first (Hold_R_Pistol on the loop's post, Hold_L_Cup under the trigger
housing at the loop's front, SOCKET_Trigger, Reload_L_Wrap on the magazines; every Tripo body checked on the sheet), then
Ranchhand (Rack_L_Pump), Drover, Rustler, Thresher (cup on the box), Longhorn, Powderkeg, the revolver.

## Risks to watch
Scale (gun 1.32x, arms 0.88x: palm offsets in hand cm); hero arms (same skeleton; Gauge's three fingers); a visible AR
cup may break OnGun's 0.85 frame limit or show an elbow: tune the rifle's framing, not the marker; ADS sight picture with
the cup; markers read live for moving parts.

## Rules for every lane
Agents never build, commit, open/close the editor, run tests, or edit CODEMAP.md / Docs. Only your own files (above). Keep
every public name other code or tests use. Files under ~500 lines. Comments say why. Tabs. Report: files changed, what you
could not do, tests changed.
