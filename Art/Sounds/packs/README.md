# Recorded packs

The user's rule (2026-10-21): royalty-free recordings are allowed for **guns and bullet impacts only**, always processed
through the recipes (`recipes/guns_shots.py`, `recipes/guns_impacts.py`, via `lib/sample.py`); every other sound stays
synthesized. 2026-10-22 (the gun feel pass): the user's yes to **reload and handling foley** from the same pack
(`recipes/guns_foley.py`), and yes in principle to one more part of the same free bundles (Main asks before any download).
No new pack without the user's word.

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
| `Shotgun_Saiga_Cocking.wav` | 2016 Pole Position - Gun Foley: `SAIGA-12_12g_solid_slug_foley_close_up_RSM191_R_cocking.wav` 0-0.8, 2.25-3 | shotgun Mech until 2026-10-22 (the rack took its clack); unused, kept |
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

The four new guns, their rocket and the reload foley (Phase 12.3, GUNS-SOUND, 2026-10-21 night; PP = 2016 Pole Position
Production, GM = 2017 Gamemaster Audio, BZ = 2017 Bluezone Corporation - War Zone (Designed Explosions)):

| Excerpt (`Sonniss/`) | Source (library: file; seconds used) | Used for |
|---|---|---|
| `SMG_MP7_Gun.wav` | PP HK MP7: `HK_MP7_4.6mm_on_gun_DPA4062_clean_Four_shots_x_1.wav` 0-0.9 | SMG Core (the front) |
| `SMG_MP7_1m.wav` | PP HK MP7: `HK_MP7_4.6mm_1m_left_off_axis_MKH416_clean_Four_shots_x_1.wav` 0-0.9 | SMG Core (bark) |
| `SMG_Uzi_1m.wav` | PP Uzi: `Uzi_9mm_1m_right_MKH8040_1_clean_Five_shots_x_1.wav` 0.15-1.6 | SMG Core (bark), Low, Distant |
| `SMG_MP5_2m.wav` | PP HK MP5: `HK_MP5_9mm_2m_behind_clean_Double_shots_x_1.wav` 0-1.2 | SMG Core (bark), Low, Distant |
| `SMG_Vz61_1m5.wav` | PP Vz61: `Vz61_Scorpion_9mm_1.5m_behind_right_clean_Double_shots_x_1.wav` 0-1.2 | SMG Core (bark), Low |
| `SMG_MP5_75m.wav` | PP HK MP5: `HK_MP5_9mm_75m_behind_Schoeps_A_clean_Double_shots_x_1.wav` 0.1-2.6 | SMG Distant, TailOpen roll |
| `SMG_Uzi_75m.wav` | PP Uzi: `Uzi_9mm_75m_behind_Ehrlund_R1_clean_Five_shots_x_1.wav` 0.25-3.2 | SMG Distant, TailOpen roll |
| `SMG_Vz61_50m.wav` | PP Vz61: `Vz61_Scorpion_9mm_50m_behind_Ehrlund_R1_clean_Double_shots_x_1.wav` 0-2.2 | SMG Distant |
| `SMG_P90_75m.wav` | PP FN P90: `FN_P90_5.7mm_75m_behind_Ehrlund_R1_clean_Double_shots_x_1.wav` 0.15-2.9 | SMG Distant |
| `SMG_TMP_Bolt.wav` | PP Gun Foley: `Steyr_TMP_9mm_foley_distant_MKH418S_S_cocking_&_dry_fire.wav` six 0.7-1.4 s windows | SMG Mech (bolt home) |
| `Foley_G36C_Mag.wav` | PP Gun Foley: `Heckler_&_Koch_G36C_5.56mm_foley_close_up_MKH60_mag_in_&_out.wav` eight 0.6-0.9 s windows | SMG HelixOut, HelixIn |
| `Sniper_Barrett_Gun.wav` | PP Barrett M98B: `Barrett_M98B_.338_Lapua_on_gun_DPA4062_clean_Single_shots_x_17.wav` six 1.05 s windows from 3.0 s | Sniper Core (front) |
| `Sniper_Barrett_2m.wav` | PP Barrett M98B: `Barrett_M98B_.338_Lapua_2m_right_MKH60_clean_Single_shots_x_17.wav` six 3.45 s windows from 2.96 s | Sniper Core (bark), Low, TailOpen roll, Distant |
| `Sniper_R700_1m.wav` | PP Remington 700: `Remington_700_7.62x51mm_1m_right_MKH8040_1_clean_Single_shots_x_14.wav` four windows 0-14.7 | Sniper Core (bark) |
| `Sniper_R700_50m.wav` | PP Remington 700: `Remington_700_7.62x51mm_50m_behind_788_Schoeps_B_clean_Single_shots_x_14.wav` four windows 0-15.5 | Sniper Distant |
| `Sniper_Sako_1m.wav` | PP Sako TRG 22: `Sako_TRG_22_TRG_1m_left_off_axis_MKH416_clean_Single_shots_x_5.wav` three windows | Sniper Core (bark) |
| `Sniper_GM_Shot.wav` | GM Gun Sound Pack: `gun_rifle_sniper_shot_01.wav` whole | Sniper Core (weight), Low |
| `Sniper_FAL_25m.wav` | PP FN FAL: `FN_FAL_7.62x51mm_25m_in_front_Holophone_Ls_clean_2_shots.wav` 0-3.4 | Sniper Distant |
| `LMG_L7A2_1m.wav` | PP L7A2 GPMG: `L7A2_GPMG_7.62x51mm_belt_fed_1m_in_front_RE20_clean_Single_shots_tracer_x_2.wav` 3.95-6.4 | LMG Core (chug), Low |
| `LMG_L110_1m.wav` | PP L110A2: `L110A2_LMG_5.56mm_belt_fed_1m_right_MKH8040_1_clean_Single_shots_x_2.wav` 0.05-1.7, 3.95-5.49 | LMG Core, Low |
| `LMG_AKM_Ground.wav` | PP AKM: `AKM_7.62x39mm_1m_left_on_ground_DPA4062_clean_Triple_shots_x_1.wav` 0.05-1.4 | LMG Core (front) |
| `LMG_MG_Field.wav` | 2017 Pole Position - The Warfare Library: `warfare_t2_mg_firing_close_projectile_tail_large_field_Telinga_w_MKH8020_or_MKH8060.wav` 0-1.24, 4.4-9, 23.4-26.45, 40.15-41.25, 44.3-52 | LMG Core, TailOpen roll, Distant |
| `LMG_L110_50m.wav` | PP L110A2: `L110A2_LMG_5.56mm_belt_fed_50m_behind_788_Schoeps_A_clean_Single_shots_x_2.wav` 0.05-3.9, 4.03-5.49 | LMG Distant |
| `LMG_L7A2_50m.wav` | PP L7A2 GPMG: `L7A2_GPMG_7.62x51mm_belt_fed_50m_behind_Schoeps_B_clean_Single_shots_tracer_x_2.wav` 0.08-4, 4.08-7.4 | LMG Distant |
| `LMG_M2_Warehouse.wav` | 2016 Audiobeast: `Audiobeast_Medium_Warehouse_Browning_M2_.50_Machine_Gun_03m_RSM191_MS_Raw_002_Burst_x2.wav` 19.4-23, 24.6-30 | LMG TailInside (after a burst) |
| `Launcher_MK44.wav` | 2016 Fascinated Sound - The Gun Locker: `Automatic Cannon - MK44 - 03 - Single Shot with Report 03.wav` 0.2-6.5 | Launcher Core (thump), Low, TailOpen roll, Distant |
| `Launcher_Whoosh.wav` | BZ: `Bluezone-BC0214-explosion-whooshe-003.wav` 0-2.33 (the whoosh before its blast, played reversed) | Launcher Core (the rocket leaving), Distant |
| `Explosion_Large.wav` | GM Explosion Sound Pack: `explosion_large_08.wav` whole | Launcher.Explosion (body) |
| `Explosion_NoTail.wav` | GM Explosion Sound Pack: `explosion_large_no_tail_03.wav` whole | Launcher.Explosion (front) |
| `Explosion_MedTail.wav` | GM Explosion Sound Pack: `explosion_med_long_tail_01.wav` whole | Launcher.Explosion (body) |
| `Explosion_Mine2.wav` | BZ: `Bluezone-BC0214-beep-mine-explosion-002.wav` 0.35-6 (after its beep) | Launcher.Explosion (body) |
| `Explosion_Whoosh.wav` | BZ: `Bluezone-BC0214-explosion-whooshe-020.wav` 2.55-8.9 (its blast) | Launcher.Explosion (body) |
| `Explosion_Far.wav` | GM Explosion Sound Pack: `explosion_far_distant_02.wav` whole | Launcher.Explosion.Distant |
| `Explosion_Cannon_Far.wav` | 2017 Pole Position - The Warfare Library: `warfare_t1b_cannon_firing_forest_distant_MKH8060_2.wav` 0.4-2.7, 2.65-7, 16.15-17.95, 17.95-22 | Launcher.Explosion.Distant |

Reload and handling foley, heavy hits (the gun feel pass, GF-SOUND-ART, 2026-10-22; PP = 2016 Pole Position Production -
Various Gun Foley & Handling, TS = 2016 TS Sound - 12 Gauge Shotgun). Each windowed excerpt's windows are the same length
with the event the same time in (`guns_foley._FOLEY`):

| Excerpt (`Sonniss/`) | Source (library: file; seconds used) | Used for |
|---|---|---|
| `Foley_G36C_MagIn.wav` | PP: `Heckler_&_Koch_G36C_5.56mm_foley_close_up_MKH60_mag_in_&_out.wav` six 0.5 s windows round the seats at 1.568, 6.612, 11.556, 16.124, 20.604, 25.376 | Rifle.MagIn, Sniper.MagIn (slowed) |
| `Foley_G36C_MagOut.wav` | PP: the same file, six 0.8 s windows round the releases at 4.214, 9.486, 14.384, 18.340, 23.286, 28.696 | Rifle.MagOut, Sniper.MagOut (slowed) |
| `Foley_Saiga_Rack.wav` | PP: `SAIGA-12_12g_solid_slug_foley_close_up_RSM191_R_cocking.wav` 0-0.8, 2.2-3.0 (two whole racks) | Rifle.Bolt |
| `Foley_Saiga_Pull.wav` | PP: the same file, six 0.35 s windows at 4.46, 7.846, 10.562, 13.492, 16.866, 19.95 | Rifle.Bolt, Rifle.LastRound, Equip.Heavy |
| `Foley_Saiga_Release.wav` | PP: the same file, six 0.5 s windows at 6.194, 9.392, 12.274, 15.382, 18.58, 21.796 | Rifle.Bolt |
| `Foley_M29_Open.wav` | PP: `Smith_&_Wesson_M29_.44_Magnum_foley_close_up_MKH416_cylinder_open_&_close.wav` six 0.32 s windows at 0.47, 5.298, 10.574, 22.06, 26.66, 38.78 | Revolver.CylinderOut, RoundsIn |
| `Foley_M29_Close.wav` | PP: the same file, six 0.31 s windows at 2.556, 7.866, 15.01, 19.69, 28.772, 36.914 | Revolver.CylinderIn, Revolver.LastRound |
| `Foley_TS_ShellLoad.wav` | TS: `Shell Load_Round Eject.wav` 2.55-3.35, 4.05-4.85 | Shotgun.ShellIn |
| `Foley_TS_Pump.wav` | TS: `Shotgun_Quick Pump_01.wav` 0-0.6, 0.98-1.48 | Shotgun.Pump, Shotgun.Rack |
| `Foley_Brass_Drop.wav` | 2016 Stuart Duffield - Bullet SFX: `Bullet_Drop_Wood_04.wav` 0.1-0.72 | Revolver.Eject |
| `Impact_PunchHeavy.wav` | 2017 Gamemaster Audio - Punch Sound Pack: `punch_heavy_huge_distorted_01.wav` whole | Creature.Hit.Heavy |

`SMG_TMP_Bolt.wav` (above) also makes Weapon.SMG.Charge; `Impact_Concrete.wav` Impact.Heavy, Impact.Scatter and
Creature.Crit.Shell; `Impact_Body.wav` and `Impact_PunchWet.wav` Creature.Hit.Heavy and Creature.Crit.Gel.
