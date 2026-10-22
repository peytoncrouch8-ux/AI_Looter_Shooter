# Pipeline: The game

Part of the pipeline notes (index: `Docs/Pipeline.md`). Rules: `CLAUDE.md`.

## The game

- Story: *Revenant* (`Docs/Story.md`), weird west on the planet Teropa. One area per Hollow, each designed in `Docs/Areas/`.
- Areas, map order since 2026-10-13:
  - 1 Ransom's Rest: a new game starts on its cold open; its first missions teach the controls by hints. Mountain expansion:
    `Docs/Areas/RansomsRest_Expansion.md`.
  - 2 the *Gilded Lily*: behind the north mountain, by train; not built.
  - 3 Skyreach (the old tutorial island): reached later from the Lily by skylift; off the station boards for now, test with
    `Looter.Travel Skyreach`.
- Missions: all but Main 1 come from a giver (`EMissionStart::Offered`, `FMissionOffer`): a speaker point, a notice board or an
  artifact (`AMissionArtifact`).
  - ? = pick up, ! = turn in (the user's call, 2026-10-13: the reverse of Borderlands). Gold = story, white = sides. Shown on the
    map, the minimap and over the giver (`UGiverMarkSubsystem`).
  - Offer card `UMissionOfferWidget`: Accept / Not now. The Missions tab lists only missions taken up.
    `Looter.Mission.Offer <id>` opens a card.
  - Features: optional objectives (`bOptional`: never hold a step, pay `OptionalSilver` at the turn-in); mission items
    (`AMissionItem`, carried, dropped by a creature); a turn-in with a choice of two (held to choose; story flags in
    `FCampaignRecord::StoryFlags`, read by `FStoryCondition::WithFlags`); readable props; the porch plate; an UNDERGROUND minimap
    plate (actors tagged `Underground`); any speaker point or a vendor's hatch as a turn-in.
- Hob's roosts (the user's pick, 2026-10-13): the ONE save / respawn / fast-travel point on every map: the Crow's Mast
  (`SM_HobRoost`, bell `SM_HobRoostBell`), beacon in Hob's Ember (#D8321C, core #FF8A52; `URoostSettings`, in Project Settings
  under Game > Hob's Roosts; `LooterUI::Color::Roost`).
  - Walk within 9 m (clear line) to find one: column shoots up, bell rings, crows lift, three circle the beacon, ROOST FOUND,
    early autosave. Continue wakes the player at the area's last found roost (`FCampaignRecord::LastGraves`).
  - Fast travel and deaths: Hob's crows carry the player across the screen (`URoostCarrySubsystem`, `UHudCrowVeilWidget`); Hob
    speaks at every death.
  - The actor is still `ARespawnMarker` (save ids unchanged). Placed from `layout.json` `gameplay.roosts` by
    `Tools/Unreal/build_area_roosts.py` (run alone it replaces every roost in the level). `Looter.Grave.List`,
    `Looter.Grave.Mark <id> | forget <id|all>`.
  - TWO a map at most (the user, 2026-10-14; `MOST_PER_MAP`): Ransom's Rest's are the Depot (train station) and Calder's Post
    (mountain); the other nine were removed, not kept for saves.
  - A death wakes at the roost last found on the map (never the nearest), before any at the level's own start (RR: the
    family plot's PlayerStart), Hob's crows carrying Ellis either way on a map with roosts. Arriving by fast travel finds
    the roost too (`MarkOnArrival`). Continue wakes at the same roost, before any where saved. One lookup:
    `ARespawnMarker::FindLastFound` (`LastGraves`, else the newest `MarkedGraves` still standing: an old save naming a gone
    roost loads onto one that stands; no migration).
- Impacts and bullet marks: a hit's surface (13: dirt, grass, gravel, sand, stone, wood, metal, sheet metal, glass, water, cloth,
  hay, bone) comes from tags, then the trim sheet's strip under the hit (collision UVs), the terrain's baked surface map,
  material-name words; no physical surface types. Each has its sound, puff and mark; glancing hits ricochet.
  - Guns' after-shot (2026-10-14): brass (`FWeaponBrass`: last 60, 20 s + 1.2 s sink, SOCKET_Eject or a per-kind port on the
    visible gun; the revolver tips its empties at the reload's Eject step; `Weapon.Casing.*` by surface); a hot barrel's wisp
    (no shimmer: a distortion pass costs too much); tracers with a hot head and a smoke thread; visible ricochets (at most 2 a
    volley); impact chunks land on the hit's floor; each creature splashes its own (`ECreatureBlood`); enemy shots passing within
    1.5 m of the player's head whiz or hiss once (`NearMiss`).
  - Spider deaths (the user, 2026-10-14: more dramatic, legs curled UNDER the body): `SpiderCreatureDeath.cpp` throe
    (rear, kicks) 0-0.4 s, drop on the cry's knock with a bounce + `Creature.Spider.Thud` + dust, legs folded under back to
    front by 1.15 s, shudder before the sink; timing scales by sqrt(size). Watch any death on cue: `Looter.SpawnCreature`,
    `slomo 0.25`, `Looter.KillNearest`, grab frames, `slomo 1`.
  - Creatures react (2026-10-14, `UCreatureReactComponent`): a notice beat once a hunt (0.42-0.62 s by seed; none when hurt,
    a boss, or called by the pack), the M_FX_HitFlash overlay on a hit, deaths that linger (spiders 10 s with twitches, slimes
    splat, the Unpaid crumble to ash), rising entrances when seen within 60 m, idle dice, footfall dust (level budget 40/s).
  - Marks are pooled DBuffer decals (`Looter.BulletMarks.Max/.Hold/.Fade`, 64, 20 s + 15 s fade); atlas script-made
    (`Art/Textures/BulletMarks/make_bullet_marks.py`).
  - Re-run `Tools/Unreal/build_impact_surfaces.py` after a macro map, scatter map or new trim-sheet model changes.
- Objective highlight: within 13 m, what the tracked mission's current objective is about (optional ones too, the giver once it
  waits; never a fight's creatures) glows orange as an overlay on its meshes (a spot of a building round a door).
  `M_ObjectiveHighlight` from `build_highlight_material.py`; `Looter.Highlight.List`. A character (a story character or a
  skinned figure, never a gun; `IsFigure`) wears the figure look: rim only, body all but clear (`Figure` 1; tune the
  script's `Figure*` defaults). A thing's 20% fill turned Sexton's near-black coat solid orange (fixed 2026-10-14).
- Holds: every hold-to-use shows the kit's round ring (`SLooterProgressRing`, `LooterUI::MakeHoldKeyCap`).
- HUD (the user's list, 2026-10-14): the mission tracker sits under the minimap, right-aligned (`TopFor`, `RightInset` 36,
  `MaxWidth` 540: clear of the boss bar's 380 px either side of the middle); steps and the objective are circles, checked
  once done; a line without a count takes the count's room before wrapping. A kill's XP floats by the kill (`XpPopup*`) and
  as "+N XP" after the XP bar's numbers. The gun's notches (inked tally in its tier's colour, count) sit before its name.
  Health and XP bars 25% longer and thicker (`HudFrameShapes::BarScale`). Subtitles On/Off in Interface
  (`Looter.Subtitles`): off hides lines with a speaker (barks too); notices stay. The game has no voice acting, so off
  means NPC talk isn't shown at all.
- Lost Loot (the user's call, 2026-10-13, so ground guns don't pile up and cost frame time): a gun on the ground lasts 5 minutes
  (not while the player stands within 6 m), then goes to the depot's LOST PROPERTY window. Leaving a map or quitting sends every
  ground gun there; a load never puts one back. 30 spaces, +10 per upgrade (five, bought with silver); the oldest unnamed gun goes
  out when full. `Looter.LostLoot.*`.
- The ore lift: `AOreLift` rides as a skippable look-only scene; the body is parked on the far landing at the start (a save,
  death or load mid-ride finds it there). `Looter.Lift.*`.
- Look: stylized realism; the user rejected faceted low-poly. Golden-hour palettes on Ransom's Rest (ochre, olive, sandstone),
  late summer. Loot colours (rarity orange, teal, gold) stay off the world.
- Third person (2026-10-14, `LooterStancePoseCarry`): low-ready after 0.8 s moving without aim/fire (~19 deg down), aim or
  fire snaps back in 0.12 s; sprint = chest carry, both hands turning with the gun (long foregrips drawn in), revolver low
  at the side one-handed; the upper body keeps ~44% of the jog's bounce. The mannequin is animated hidden; Ellis wears
  its pose (2026-10-15, "Character models", "Ellis as the player's body").
- The idle (2026-10-15, the user's picks: "Weary revenant" with breathing, glances around and hand fidgets; the armed
  stance kept; `LooterStancePoseIdle.cpp`, on the mannequin before the crouch, so Ellis copies it): standing still
  (under 10 cm/s, grounded, not crouched, sprinting, sliding or climbing) and unarmed it eases in over 1.2 s, out over
  0.25 s. Its own clock (`DescribeIdle`, the same moment always the same idle): the weight from leg to leg (18 s round,
  held near each end; the hips 2.5 cm over the standing leg, the free hip 3 degrees down, the chest 2 back, the legs
  solved back onto the planted feet), weary (chest 4 degrees forward, shoulders rounded 5, neck 7 down, head 2 back up),
  a breath every 4.2 s (chest 0.8, shoulders 1.3), a glance aside in about three of four 10 s spells (18-35 degrees,
  neck 40%), a finger flex now and then (22 degrees a joint toward the palm), the right thumb in the belt for 5-9 s in
  about two of three 24 s spells (the wrist 15 out, 12 ahead, 8 up from the hips; fingers down, palm to the body).
  First person is unaffected (the arms ride the capsule). Tests `Looter.PlayerAnim.Idle.*`.
- First-person hands (2026-10-14, `UFirstPersonHandsComponent`, the user's pick: full, like Borderlands): arms on the
  camera posed after the gun each frame; per-kind hold table in `FirstPersonHandsGrips` (where tuning happens); the support
  hand follows reload parts and carries the jar. SK_FP_Arms (approved 2026-10-14; `Art/Models/Characters/FPArms.py`,
  sheet `Saved/Concepts/Hands/fp_arms_sheet.png`) at `ArmsScale` 0.88 (the user: hands looked big). The rig reads bone
  positions only (the import turns bone axes). Hip holds (`Weapons/WeaponHipHold.cpp`; the revolver's in
  `create_revolver.py`): grip cm ahead/right/up, rifle 50, 23, -23.5; shotgun 30, 22.5, -23; revolver 50, 22, -21 (the
  user's Borderlands framing: low right, both gloves in view; `Looter.Player.Hands.OnGun` keeps the support grip point inside
  0.85 of the half-frame, the trigger's inside 1.0 (but for the shotgun's); the Borderlands framing nudges these, below). Long guns level (no muzzle-down tilt: the user, "the AR is
  tilted forward"). Support hand palm up under the fore-end, forearms from below so the sleeve's cut end (SK_FP_Arms
  ends at the elbow in a capped cut) stays under the frame (`ForeForearm` in `FirstPersonHandsGrips.cpp`).
- Long forearms (2026-10-22, `Saved/FPHands/PLAN.md` fix E): every SK_FP_Arms* forearm (elbow to wrist) is 1.25x Ellis's,
  32.62 cm (`FPArms.py` `FOREARM_STRETCH`; 26.10 real), so the framing can slant it with the elbow out of the frame.
  Shoulder, upper arm, elbow and the hand's rest turn are Ellis's; the wrist (hand with it) moves out along the forearm's
  line. Hands, cuffs, gauntlets, upper arms unchanged in size. Triangles unchanged (8018; Odessa 11676, Crane 10724,
  Gauge 10212, Pike 10412): the sleeves' upper-arm rings went 2.8 -> 6.2 cm apart (`UPPER_STEP`) to pay for the
  forearm's. Rig jsons: `lowerarm_cm` 32.62, `lowerarm_body_cm` 26.1, `forearm_stretch`; the code reads the length
  from the skeleton (`FFirstPersonHandsRig::GetForearmLength`). Preview: `Saved/ArtPreviews/Characters/FPArms_Long_<Hero>.png`.
- The heroes' first-person arms (2026-10-20): SK_FP_Arms_Odessa, _Crane, _Gauge, _Pike from
  `Art/Models/Characters/FPArms<Hero>.py` on `Tools/Blender/fp_arms_heroes.py` (+ `fp_arms_base/hands/cloth/paint/gauge.py`;
  FPArms.py's definitions are run up to its build line, FPArms.py never edited). Every one is on SK_FP_Arms' skeleton bone
  for bone (`Intermediate/FPArms/FPArms_<Hero>_rig.json` = FPArms_rig.json), so the rig, the grip tables and the motions
  serve all five unchanged; palms' undersides kept within 2 mm of Ellis's (the 3.6 cm grip offset). Looks: each hero's own
  procedural looks (from their model's tables) baked onto these meshes raised 1.5 m (dust and mud go by height), then fitted
  to first person (`BakedSet.adjust`: the 3P creases read as crazed plastic a foot from the eye: bumps 0.3-0.85, skin and
  glove colour softened), then the hand's detail painted (seams and points on a glove; nails, knuckle wrinkles, palm lines
  on bare hands); texels baked tilted past 60 degrees (decimation fold-overs) are flattened. Gauge's three fingers: blocks on
  `thumb_*`/`index_*`/`middle_*`, moved 6-7 mm apart along their curl axes (the skeleton can't be spread); ring and pinky
  bones unweighted. Worn by `UFirstPersonHandsComponent::LoadArms` from the played hero (`UHeroDefinition::FirstPersonArms`,
  naming rule SK_FP_Arms_<Id>; Ellis's while a hero's aren't in), at `FirstPersonArmsScale` (Odessa 0.84, Pike 0.86, the
  rest 0: the component's 0.88; `create_ember_assets.py` `ARMS_SCALE`); `Looter.Hero` mid-play swaps them (`WearHero`).
  Look sheet: `Tools/Blender/fp_arms_heroes_sheet.py <Hero>` (fp_arms_sheet.py's poser on the three guns at the hip holds
  and drawn sizes) into `Saved/ArtPreviews/Characters/FPArms_<Hero>/`. Export `artrun.ps1 -Export FPArms<Hero> -Family
  FPArms<Hero>`; the bake is skipped while `Art/Textures/FPArms<Hero><Set>/bake.digest` matches. Test
  `Looter.Player.Hands.HeroArms`. Wiring notes: `Saved/FPArms/WIRING.md`.
- The shotgun (the user, 2026-10-15): 20 cm nearer than the rifle ("too far forward making the arm look silly"): the
  stock leaves the frame past the bottom right and the left arm's reach is short. Every full stock hid the trigger glove
  at the old hold (only the stockless Raider grip showed it), so it sits under the frame now. The left hand cups the pump
  from the lower left, rolled 20 degrees and turned 45 in its own plane, forearm in line (straight down bent the wrist
  square: "angles abnormally"), the pump through the grip's middle, `EHandGrip::Pump` curls (fingers closed round it,
  the thumb straight along its near side: curled, it read as "a nub").
- Tune hands live in PIE: `Looter.HipHold.Kind/Ahead/Right/Up/Pitch/Yaw/Roll`, `Looter.Hands.Kind`,
  `Looter.Hands.Support*/Trigger*` (palm roll/pitch/yaw, forearm yaw/pitch, hold), `Looter.Hands.SupportFingerCurl` and
  `SupportThumbCurl` (every joint, degrees), `Looter.Hands.Scale`; `Looter.Hands.Dump` prints paste-ready table lines.
  The forearm switches barely move a far hand: past the upper arm's reach the shoulder comes to the elbow (90 cm at
  most), so move the hold instead. `Looter.GiveWeapon <gun> <rarity> <level> Slot=Key` picks parts; `play.py equip <n>`
  takes a slot in hand (number keys don't reach the game through input.ps1).
- Borderlands framing (FP-HANDS, 2026-10-22; the user: "hands show too much of the arms, clip into the guns"):
  `Player/FirstPersonHandsFraming` on top of the hip and grip tables; every number a live `Looter.FPHands.*` cvar,
  `Looter.FPHands.Dump` prints bake lines, `Looter.FPHands.Enable 0` = as before. Borderlands at 16:9 (measured, sources
  in `Saved/FPHands/PLAN.md`): muzzle 0.54-0.63 across, 0.52-0.64 down, gun to the bottom-right corner; only gloves and
  wrists show (support glove 0.80-1.0 down; BL3 sometimes a 50-60 deg slanted gauntlet), never an elbow; pistols
  one-handed. Ours now: forearms `FrameForearm` (most slanted way from 50 deg toward straight down whose elbow leaves the
  frame by 0.06 of the half-frame, 0.6 of it turned back toward the eye's plane, at most 60 deg off the table's way);
  per kind (baked in `MakeDefaults`): Rifle -12 ahead -2 right -7 up pitch +5, trigger glove 2.5 cm right (no finger
  through the Bullpup's thumbhole; its support can't slide back: grip loop at u 37.8); Shotgun -1.5 up, support -3;
  Revolver -5 right +1 up roll +6, one-handed (support parked at 20, -26, -38 cm, eased 0.15 s, back on the gun for
  reloads and throws); SMG -1.5 up, support -2; Sniper -6 ahead -3.5 up pitch +2, support -6; LMG -10 ahead -3.5 up pitch
  +4, support -6 (to u 38); Launcher +9 ahead +2 right -2 up. "Pull the guns in" (the user, 2026-10-22): a support wrist
  high in the frame (fore-ends ~70 cm out) could only hide its elbow by running straight down, so the rifle, LMG and
  sniper sit nearer and lower with the muzzle tipped up about the grip (never down: the user's 2026-10-14 call), muzzle
  0.60-0.64 down, support glove 0.78-0.84; forearm on screen rifle 20% of the height (was 29), LMG 18 (22), sniper 12
  (19), shotgun/launcher 17, SMG 14 (`Looter.FPHands.Play.OnScreen` logs it); ADS unchanged (sight on the crosshair:
  the aim slerps from the hip hold); sheet `Saved/FPHands/compare2.png`. `ViewFov` 75 (narrower view model, holds
  pushed out by `DepthScale`) flattens the gun but not the arms: left off. Framing checks: `Looter.FPHands.PreviewAspect
  1.7778` draws PIE at 16:9 inside the 2.17:1 editor viewport (bars at the sides; grab the middle: x 0.093-0.893, y
  0.132-0.962 of the window); before/after sheet `Saved/FPHands/compare.png`.
- Gun sizes (the user, 2026-10-14): drawn bigger than modelled, everywhere: rifle and shotgun 1.1, revolver 1.2
  (`UWeaponModelComponent::ScaleFor`; sockets scale with it). The pistol shows as "Revolver" (DA_Revolver's display name;
  Drover stays its parts' and files' name).
- Swaps (2026-10-14, `Player/WeaponSwapMotion`, `Inventory/WeaponManagerSwap.cpp`): the gun in hand drops and tips out
  of view (~0.19-0.24 s by handling, `Weapon.Holster` / `Weapon.Revolver.Holster`), the guns change out of sight, the
  next comes up with an overshoot and settle; a second press mid-holster retargets; no sights while swapping;
  `Looter.Swap.Motion 0` turns it off. Third person: the right arm lowers the gun, the left lets go.
- Water (2026-10-14, the user's concept, clear teal): water meshes carry vertex R shore, G depth, B/A flow (layout x, y);
  `M_Water` reads them (opaque, lit, 3 taps; defaults tuned: RippleScale 0.2, RippleStrength 0.3, Roughness 0.12, SkyTint
  0.35, DepthPower 0.6, shallow #43918A, deep #184A54); `M_Waterfall` streaks in world metres; `M_WaterMist` (Opacity 0.35,
  Brightness 1.6). Rebuild only the water: `$env:LOOTER_TERRAIN_ONLY='water'; Tools\models.ps1 -Only <Areas>`, then
  `build_area.py <Area> water` (re-places the falls in `Effects/Falls`). terrain_identity differs only in `<Area>_Water`
  until committed. Splash rings and wake: `FWaterRings`.
- Player numbers (2026-10-13): 0.85 of the mannequin's size and 0.9 pace on top (the user: "pace of play is super fast"): walk
  459, sprint 711, crouch 230, slide 783 cm/s; eye about 140 cm; jump 103.5 cm. A slide comes out of sprint plus crouch. Chasing
  creatures scale with `LooterPlayerSize::MoveScale` (size and pace); slimes don't.
