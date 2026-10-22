# Recorded packs

The user's rule (2026-10-21): royalty-free recordings are allowed for **guns and bullet impacts only**, always processed
through the recipes (`recipes/guns_shots.py`, `recipes/guns_impacts.py`, via `lib/sample.py`); every other sound stays
synthesized. No new pack without the user's word.

## Licence (Sonniss GDC Game Audio Bundles 2016 and 2017)

`Sonniss/Licensing.pdf` (the same file in both years; current terms: https://sonniss.com/gdc-bundle-license/): use in
games and other productions, commercial included; edit freely; no attribution needed; **no** resale, redistribution or
sharing of the sounds as sounds (a sound library, a pack, a marketplace); **no** AI training. The excerpts here sit in
the team's private repository (allowed: it is the licensee's own use); never publish the `packs/` folder or the raw files.

## Files

The raw bundles stay outside git in `Saved/SoundPacks/Sonniss/<year>/<Vendor - Library>/` (59 files, listed in
`Saved/Gunfeel/SONNISS_FILES.txt`). `make_pack.py` cuts the parts the recipes use into `Sonniss/` (48 kHz, 24-bit, several
windows of a long take joined by 0.25 s of silence) and prints each excerpt's shots; the recipes list the shots they use
(`guns_shots.SHOTS`). Run it again after changing its table:

    "C:\Program Files\Blender Foundation\Blender 4.4\4.4\python\bin\python.exe" -I Art/Sounds/packs/make_pack.py

| Excerpt (`Sonniss/`) | Source (year, vendor - library: file; seconds used) | Used for |
|---|---|---|
| `Rifle_FAMAS_1m.wav` | 2016 Pole Position - FAMAS G2 5.56mm: `FAMAS_G2_5.56mm_1m_left_on_ground_DPA4062_clean_Four_shots_x_1.wav` 0-1.5 | rifle Core (the front) |
| `Rifle_L85_1m.wav` | 2016 Pole Position - Enfield L85: `Enfield_L85_rifle_5.56mm_1m_left_off_axis_MKH416_clean_Five_shots_x_1.wav` 0-1.5 | rifle Core (body), Low |
| `Rifle_HK416_1m.wav` | 2016 Pole Position - HK416: `HK_416_5.56mm_1m_left_off_axis_MKH416_clean_Six_shots_x_2.wav` 0-1.6 | rifle Core (body) |
| `Rifle_Tavor_1m.wav` | 2016 Pole Position - IMI Tavor TAR-21: `IMI_Tavor_TAR-21_5.56mm_1m_left_off_axis_MKH416_clean_Triple_shots_x_1.wav` 0-1.3 | rifle Low |
| `Rifle_G36C_50m.wav` | 2016 Pole Position - HK G36C: `Heckler_&_Koch_G36C_5.56mm_50m_behind_Ehrlund_R2_clean_Six_shots_x_1.wav` 0-3 | rifle Distant, TailOpen roll |
| `Rifle_FAMAS_55m.wav` | 2016 Pole Position - FAMAS G2: `FAMAS_G2_5.56mm_55m_in_front_off_axis_clean_Six_shots_x_1.L.wav` 0-2.8 | rifle Distant |
| `Rifle_Tavor_20m.wav` | 2016 Pole Position - IMI Tavor: `IMI_Tavor_TAR-21_5.56mm_20m_front_right_NT4_R_clean_Triple_shots_x_1.wav` 0-2 | rifle Distant, TailOpen roll |
| `Rifle_HK416_Room.wav` | 2016 Audiobeast - London Warehouse: `Audiobeast_Small_Room_H&K_416_5.56_Assault_Rifle_05m_LS5_001_Single_Shot_x1.wav` 3.75-6.6 | rifle TailInside |
| `Rifle_Cock.wav` | 2017 Gamemaster Audio - Gun Sound Pack: `gun_semi_auto_rifle_cock_02.wav` whole | rifle Mech (bolt home) |
| `Shotgun_Saiga_1m.wav` | 2016 Pole Position - Saiga-12: `SAIGA-12_12g_solid_slug_1m_left_off_axis_MKH416_clean_Single_shots_x_5.wav` 7.95-10.4, 11.95-14.4, 15.95-18.4, 27.95-30.4, 31.95-34 | shotgun Core (blast), Distant |
| `Shotgun_SPAS12_1m.wav` | 2016 Pole Position - SPAS-12: `SPAS-12_12g_solid_slug_1m_right_MKH8040_1_clean_Double_shots_x_1.wav` 0-2.65 | shotgun Core (boom), Low |
| `Shotgun_USAS12_1m.wav` | 2016 Pole Position - USAS-12: `USAS-12_12g_solid_slug_1m_right_CS3_clean_Double_shots_x_1.wav` 0-2.47 | shotgun Core (boom), Low |
| `Shotgun_TS_Slug.wav` | 2016 TS Sound - 12 Gauge Shotgun: `Shotgun_Fire_Slug_01.wav` whole | shotgun Core (weight), Low, TailOpen roll, Distant |
| `Shotgun_TS_Fire.wav` | 2016 TS Sound - 12 Gauge Shotgun: `Shotgun_Fire_Round Eject_01.wav` 0-3.3 | shotgun Core (weight) |
| `Shotgun_SPAS12_50m.wav` | 2016 Pole Position - SPAS-12: `SPAS-12_12g_solid_slug_50m_behind_Ehrlund_R1_clean_Double_shots_x_1.wav` 0-2.65 | shotgun Distant, TailOpen roll |
| `Shotgun_USAS12_55m.wav` | 2016 Pole Position - USAS-12: `USAS-12_12g_solid_slug_55m_in_front_off_axis_clean_Double_shots_x_1.wav` 0-2.47 | shotgun Distant, TailOpen roll |
| `Shotgun_870_Warehouse.wav` | 2016 Audiobeast: `Audiobeast_Medium_Warehouse_Remington_870_12g_Pump_Action_Shotgun_10m_U87_001_Single_Shot_x8.wav` 22.2-26.6, 28.9-33.3, 38.5-42.9, 52.8-57.2 | shotgun TailInside |
| `Shotgun_Saiga_Cocking.wav` | 2016 Pole Position - Gun Foley: `SAIGA-12_12g_solid_slug_foley_close_up_RSM191_R_cocking.wav` 0-0.8, 2.25-3 | shotgun Mech |
| `Revolver_M29_1m5.wav` | 2016 Pole Position - S&W M29 .44: `Smith_&_Wesson_M29_.44_Magnum_1.5m_behind_right_clean_Double_shots_x_4.wav` 0-1, 1-2.6, 4-5, 5-6.6, 8-9.6, 9.6-11.2 | revolver Core (bark), TailOpen roll, Distant |
| `Revolver_M29_2m_PZM.wav` | 2016 Pole Position - S&W M29 .44: `Smith_&_Wesson_M29_.44_Magnum_2m_behind_right_PZM_clean_Double_shots_x_4.wav` same windows | revolver Core (weight), Low |
| `Revolver_M66_50m.wav` | 2016 Pole Position - S&W M66 .357: `Smith_&_Wesson_M66_.357_Magnum_50m_behind_Ehrlund_L1_clean_Double_shots_x_2.wav` 0-3, 3.95-6 | revolver Distant |
| `Revolver_GM_Shot.wav` | 2017 Gamemaster Audio - Gun Sound Pack: `gun_revolver_pistol_shot_04.wav` whole | revolver Core (low), Low |
| `Revolver_Colt_Warehouse.wav` | 2016 Audiobeast: `Audiobeast_Large_Warehouse_Colt_M1911_.45_ACP_Pistol_10m_MKH8040_Pair_001_Single_Shot_x6.wav` 6.85-10.4, 15.65-19.2, 22.7-26.3, 30.5-34 | revolver and rifle TailInside |
| `Revolver_M29_Cylinder.wav` | 2016 Pole Position - Gun Foley: `Smith_&_Wesson_M29_.44_Magnum_foley_close_up_MKH416_cylinder_open_&_close.wav` six 0.5-0.7 s windows | revolver Mech |
| `Impact_Body.wav` | 2017 Gamemaster Audio - Bullet Impact Sounds: `bullet_impact_body_thump_02.wav` | Creature.Hit, Spider.Hit |
| `Impact_Concrete.wav` | 2017 Gamemaster Audio - Bullet Impact Sounds: `bullet_impact_concrete_brick_01.wav` | Impact.Stone, Impact.World (stone takes) |
| `Impact_MetalHeavy.wav` | 2017 Gamemaster Audio - Bullet Impact Sounds: `bullet_impact_metal_heavy_08.wav` | Impact.Metal, Weapon.Revolver.CritRing |
| `Impact_PunchWet.wav` | 2017 Gamemaster Audio - Punch Sound Pack: `punch_grit_wet_impact_05.wav` | Creature.Slime.Hit |
| `Impact_PunchBody.wav` | 2017 Gamemaster Audio - Punch Sound Pack: `punch_general_body_impact_03.wav` | Creature.Hit |
| `Bullet_FlyBy.wav` | 2017 Gamemaster Audio - Bullet Impact Sounds: `bullet_flyby_fast_05.wav` | Weapon.NearMiss.Whiz |
| `Bullet_Cracks.wav` | 2017 Pole Position - The Warfare Library: `warfare_t3_mg_whizzes_ricochets_bullet_cracks_M10.wav` 129.95-131.35 | Weapon.NearMiss.Whiz (supersonic crack) |
| `Licensing.pdf` | 2016/2017 bundles' licence (identical) | |
