"""Copies the parts of the recorded packs the recipes use into Art/Sounds/packs/<Pack>/ (Git LFS), so a render works from
a fresh clone: each excerpt trimmed to the shots it needs (several windows of a long take joined by a short silence),
brought to 48 kHz and 24-bit, and listed in README.md with its source. The raw packs stay in Saved/SoundPacks (outside
git). Run with Blender's Python (no system Python on the PC), from anywhere:

    "C:\\Program Files\\Blender Foundation\\Blender 4.4\\4.4\\python\\bin\\python.exe" -I Art/Sounds/packs/make_pack.py

It prints each excerpt's shots as the recipes' slicer finds them (lib/sample.onsets), to check against the tables in
recipes/guns_rec.py.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SOUNDS = os.path.dirname(HERE)
ROOT = os.path.dirname(os.path.dirname(SOUNDS))
sys.path.insert(0, SOUNDS)

import numpy as np  # noqa: E402

from lib import sample as S  # noqa: E402
from lib.core import SR, fade  # noqa: E402

RAW = os.path.join(ROOT, 'Saved', 'SoundPacks', 'Sonniss')
OUT = os.path.join(HERE, 'Sonniss')
GAP = 0.25

PP = '2016/Pole Position Production - '
GM = '2017/Gamemaster Audio -  '
AB = '2016/Audiobeast - The London Warehouse Firearms Library/'
TS = '2016/TS Sound - 12 Gauge Shotgun/'
DT = '2017/Double Trouble Audio - Wood Impacts and Debris/'
MW = '2017/Mechanical Wave - Rock,Brick and Dirt - Part 02/'
GB = '2016/SoundBits - Just Gore - Add On/'

# Excerpt name: (source under Saved/SoundPacks/Sonniss, windows (start, end) in seconds, channels: 'keep' or 'mono').
EXCERPTS = {
    # The bullpup rifle: near (1 m on the ground, 1 m off axis), far, a small room, its bolt.
    'Rifle_FAMAS_1m.wav': (PP + 'FAMAS G2 5.56mm/FAMAS_G2_5.56mm_1m_left_on_ground_DPA4062_clean_Four_shots_x_1.wav',
                           [(0.0, 1.5)], 'keep'),
    'Rifle_L85_1m.wav': (PP + 'Enfield L85 Rifle 5.56mm/Enfield_L85_rifle_5.56mm_1m_left_off_axis_MKH416_clean_Five_shots_x_1.wav',
                         [(0.0, 1.5)], 'keep'),
    'Rifle_HK416_1m.wav': (PP + ' Heckler & Koch HK416 5.56mm/HK_416_5.56mm_1m_left_off_axis_MKH416_clean_Six_shots_x_2.wav',
                           [(0.0, 1.6)], 'keep'),
    'Rifle_Tavor_1m.wav': (PP + 'IMI Tavor TAR-21 5.56mm/IMI_Tavor_TAR-21_5.56mm_1m_left_off_axis_MKH416_clean_Triple_shots_x_1.wav',
                           [(0.0, 1.3)], 'keep'),
    'Rifle_G36C_50m.wav': (PP + 'Heckler & Koch G36C 5.56mm/Heckler_&_Koch_G36C_5.56mm_50m_behind_Ehrlund_R2_clean_Six_shots_x_1.wav',
                           [(0.0, 3.0)], 'keep'),
    'Rifle_FAMAS_55m.wav': (PP + 'FAMAS G2 5.56mm/FAMAS_G2_5.56mm_55m_in_front_off_axis_clean_Six_shots_x_1.L.wav',
                            [(0.0, 2.8)], 'keep'),
    'Rifle_Tavor_20m.wav': (PP + 'IMI Tavor TAR-21 5.56mm/IMI_Tavor_TAR-21_5.56mm_20m_front_right_NT4_R_clean_Triple_shots_x_1.wav',
                            [(0.0, 2.0)], 'keep'),
    'Rifle_HK416_Room.wav': (AB + 'Audiobeast_Small_Room_H&K_416_5.56_Assault_Rifle_05m_LS5_001_Single_Shot_x1.wav',
                             [(3.75, 6.6)], 'keep'),
    'Rifle_Cock.wav': (GM + 'Gun Sound Pack/gun_semi_auto_rifle_cock_02.wav', [(0.0, 0.6)], 'keep'),
    # The pump shotgun: near (1 m), a studio slug shot with its long field, far, a warehouse.
    'Shotgun_Saiga_1m.wav': (PP + 'Saiga-12 12g solid slug/SAIGA-12_12g_solid_slug_1m_left_off_axis_MKH416_clean_Single_shots_x_5.wav',
                             [(7.95, 10.4), (11.95, 14.4), (15.95, 18.4), (27.95, 30.4), (31.95, 34.0)], 'keep'),
    'Shotgun_SPAS12_1m.wav': (PP + 'SPAS-12 12g solid slug/SPAS-12_12g_solid_slug_1m_right_MKH8040_1_clean_Double_shots_x_1.wav',
                              [(0.0, 2.65)], 'keep'),
    'Shotgun_USAS12_1m.wav': (PP + 'USAS-12 12g solid slug/USAS-12_12g_solid_slug_1m_right_CS3_clean_Double_shots_x_1.wav',
                              [(0.0, 2.47)], 'keep'),
    'Shotgun_TS_Slug.wav': (TS + 'Shotgun_Fire_Slug_01.wav', [(0.0, 5.7)], 'keep'),
    'Shotgun_TS_Fire.wav': (TS + 'Shotgun_Fire_Round Eject_01.wav', [(0.0, 3.3)], 'keep'),
    'Shotgun_SPAS12_50m.wav': (PP + 'SPAS-12 12g solid slug/SPAS-12_12g_solid_slug_50m_behind_Ehrlund_R1_clean_Double_shots_x_1.wav',
                               [(0.0, 2.65)], 'keep'),
    'Shotgun_USAS12_55m.wav': (PP + 'USAS-12 12g solid slug/USAS-12_12g_solid_slug_55m_in_front_off_axis_clean_Double_shots_x_1.wav',
                               [(0.0, 2.47)], 'mono'),
    'Shotgun_870_Warehouse.wav': (AB + 'Audiobeast_Medium_Warehouse_Remington_870_12g_Pump_Action_Shotgun_10m_U87_001_Single_Shot_x8.wav',
                                  [(22.2, 26.6), (28.9, 33.3), (38.5, 42.9), (52.8, 57.2)], 'keep'),
    'Shotgun_Saiga_Cocking.wav': (PP + 'Various Gun Foley & Handling/SAIGA-12_12g_solid_slug_foley_close_up_RSM191_R_cocking.wav',
                                  [(0.0, 0.8), (2.25, 3.0)], 'keep'),
    # The revolver: a .44 at 1.5 m and 2 m (the same shots on two microphones), a .357 far, a warehouse, the cylinder.
    'Revolver_M29_1m5.wav': (PP + 'Smith & Wesson M29 .44 Magnum/Smith_&_Wesson_M29_.44_Magnum_1.5m_behind_right_clean_Double_shots_x_4.wav',
                             [(0.0, 1.0), (1.0, 2.6), (4.0, 5.0), (5.0, 6.6), (8.0, 9.6), (9.6, 11.2)], 'mono'),
    'Revolver_M29_2m_PZM.wav': (PP + 'Smith & Wesson M29 .44 Magnum/Smith_&_Wesson_M29_.44_Magnum_2m_behind_right_PZM_clean_Double_shots_x_4.wav',
                                [(0.0, 1.0), (1.0, 2.6), (4.0, 5.0), (5.0, 6.6), (8.0, 9.6), (9.6, 11.2)], 'keep'),
    'Revolver_M66_50m.wav': (PP + 'Smith & Wesson M66 .357 Magnum/Smith_&_Wesson_M66_.357_Magnum_50m_behind_Ehrlund_L1_clean_Double_shots_x_2.wav',
                             [(0.0, 3.0), (3.95, 6.0)], 'keep'),
    'Revolver_GM_Shot.wav': (GM + 'Gun Sound Pack/gun_revolver_pistol_shot_04.wav', [(0.0, 2.2)], 'mono'),
    'Revolver_Colt_Warehouse.wav': (AB + 'Audiobeast_Large_Warehouse_Colt_M1911_.45_ACP_Pistol_10m_MKH8040_Pair_001_Single_Shot_x6.wav',
                                    [(6.85, 10.4), (15.65, 19.2), (22.7, 26.3), (30.5, 34.0)], 'mono'),
    'Revolver_M29_Cylinder.wav': (PP + 'Various Gun Foley & Handling/Smith_&_Wesson_M29_.44_Magnum_foley_close_up_MKH416_cylinder_open_&_close.wav',
                                  [(0.3, 0.8), (2.4, 2.9), (5.1, 5.6), (7.7, 8.4), (10.4, 10.9), (12.7, 13.2)], 'keep'),
    # Bullets: the body thump, punches for flesh and gel, concrete, heavy metal, a fly-by, supersonic cracks overhead.
    'Impact_Body.wav': (GM + 'Bullet Impact Sounds/bullet_impact_body_thump_02.wav', [(0.0, 0.47)], 'mono'),
    'Impact_Concrete.wav': (GM + 'Bullet Impact Sounds/bullet_impact_concrete_brick_01.wav', [(0.0, 0.35)], 'mono'),
    'Impact_MetalHeavy.wav': (GM + 'Bullet Impact Sounds/bullet_impact_metal_heavy_08.wav', [(0.0, 0.82)], 'mono'),
    'Impact_PunchWet.wav': (GM + 'Punch Sound Pack/punch_grit_wet_impact_05.wav', [(0.0, 0.51)], 'mono'),
    'Impact_PunchBody.wav': (GM + 'Punch Sound Pack/punch_general_body_impact_03.wav', [(0.0, 0.48)], 'mono'),
    'Bullet_FlyBy.wav': (GM + 'Bullet Impact Sounds/bullet_flyby_fast_05.wav', [(0.0, 0.51)], 'keep'),
    # The Rustler SMG (2026-10-21 night): an MP7 on the gun (the crisp front) and at 1 m, an Uzi, an MP5 and a Vz61 near
    # (the bark), far takes of the MP5, Uzi, Vz61 and P90, the TMP's bolt, the G36C's polymer magazine.
    'SMG_MP7_Gun.wav': (PP + ' Heckler & Koch MP7 4.6mm/HK_MP7_4.6mm_on_gun_DPA4062_clean_Four_shots_x_1.wav', [(0.0, 0.9)], 'mono'),
    'SMG_MP7_1m.wav': (PP + ' Heckler & Koch MP7 4.6mm/HK_MP7_4.6mm_1m_left_off_axis_MKH416_clean_Four_shots_x_1.wav', [(0.0, 0.9)], 'mono'),
    'SMG_Uzi_1m.wav': (PP + 'Uzi 9mm/Uzi_9mm_1m_right_MKH8040_1_clean_Five_shots_x_1.wav', [(0.15, 1.6)], 'mono'),
    'SMG_MP5_2m.wav': (PP + ' Heckler & Koch MP5 9mm/HK_MP5_9mm_2m_behind_clean_Double_shots_x_1.wav', [(0.0, 1.2)], 'mono'),
    'SMG_Vz61_1m5.wav': (PP + 'Vz61 Scorpion 9mm/Vz61_Scorpion_9mm_1.5m_behind_right_clean_Double_shots_x_1.wav', [(0.0, 1.2)], 'mono'),
    'SMG_MP5_75m.wav': (PP + ' Heckler & Koch MP5 9mm/HK_MP5_9mm_75m_behind_Schoeps_A_clean_Double_shots_x_1.wav', [(0.1, 2.6)], 'mono'),
    'SMG_Uzi_75m.wav': (PP + 'Uzi 9mm/Uzi_9mm_75m_behind_Ehrlund_R1_clean_Five_shots_x_1.wav', [(0.25, 3.2)], 'mono'),
    'SMG_Vz61_50m.wav': (PP + 'Vz61 Scorpion 9mm/Vz61_Scorpion_9mm_50m_behind_Ehrlund_R1_clean_Double_shots_x_1.wav', [(0.0, 2.2)], 'mono'),
    'SMG_P90_75m.wav': (PP + 'FN P90 5.7mm/FN_P90_5.7mm_75m_behind_Ehrlund_R1_clean_Double_shots_x_1.wav', [(0.15, 2.9)], 'mono'),
    'SMG_TMP_Bolt.wav': (PP + 'Various Gun Foley & Handling/Steyr_TMP_9mm_foley_distant_MKH418S_S_cocking_&_dry_fire.wav',
                         [(0.05, 0.75), (3.95, 4.65), (8.6, 9.7), (12.0, 13.4), (17.9, 18.7), (28.0, 28.9)], 'mono'),
    'Foley_G36C_Mag.wav': (PP + 'Various Gun Foley & Handling/Heckler_&_Koch_G36C_5.56mm_foley_close_up_MKH60_mag_in_&_out.wav',
                           [(1.0, 1.9), (3.6, 4.5), (6.2, 6.9), (9.05, 9.8), (11.2, 11.8), (14.0, 14.7), (15.6, 16.4), (17.9, 18.6)],
                           'mono'),
    # The Longhorn sniper: a Barrett .338 on the gun (the front) and at 2 m (the body and its long field), a Remington 700
    # and a Sako TRG near, the Remington at 50 m, an FN FAL at 25 m, a designed sniper shot's weight.
    'Sniper_Barrett_Gun.wav': (PP + 'Barrett M98B .338 Lapua/Barrett_M98B_.338_Lapua_on_gun_DPA4062_clean_Single_shots_x_17.wav',
                               [(s - 0.05, s + 1.0) for s in (3.049, 7.044, 11.043, 15.043, 19.044, 23.043)], 'mono'),
    'Sniper_Barrett_2m.wav': (PP + 'Barrett M98B .338 Lapua/Barrett_M98B_.338_Lapua_2m_right_MKH60_clean_Single_shots_x_17.wav',
                              [(s - 0.05, s + 3.4) for s in (3.014, 7.013, 11.021, 15.024, 19.028, 23.029)], 'mono'),
    'Sniper_R700_1m.wav': (PP + 'Remington 700 7.62x51mm/Remington_700_7.62x51mm_1m_right_MKH8040_1_clean_Single_shots_x_14.wav',
                           [(max(0.0, s - 0.05), s + 2.6) for s in (0.085, 4.081, 8.081, 12.080)], 'mono'),
    'Sniper_R700_50m.wav': (PP + 'Remington 700 7.62x51mm/Remington_700_7.62x51mm_50m_behind_788_Schoeps_B_clean_Single_shots_x_14.wav',
                            [(max(0.0, s - 0.06), s + 3.4) for s in (0.100, 4.097, 8.097, 12.096)], 'mono'),
    'Sniper_Sako_1m.wav': (PP + 'Sako TRG 22/Sako_TRG_22_TRG_1m_left_off_axis_MKH416_clean_Single_shots_x_5.wav',
                           [(s - 0.05, s + 2.5) for s in (0.199, 4.009, 12.007)], 'mono'),
    'Sniper_GM_Shot.wav': (GM + 'Gun Sound Pack/gun_rifle_sniper_shot_01.wav', [(0.0, 2.04)], 'mono'),
    'Sniper_FAL_25m.wav': (PP + 'FN FAL 7.62x51mm/FN_FAL_7.62x51mm_25m_in_front_Holophone_Ls_clean_2_shots.wav', [(0.0, 3.4)], 'mono'),
    # The Thresher LMG: an L7A2 7.62 and an L110A2 (Minimi) at 1 m, an AKM on the ground (the front), a machine gun firing
    # in a large field (single shots and its long field after a burst), the two LMGs at 50 m, a Browning M2 in a warehouse.
    'LMG_L7A2_1m.wav': (PP + 'L7A2 GPMG 7.62x51mm/L7A2_GPMG_7.62x51mm_belt_fed_1m_in_front_RE20_clean_Single_shots_tracer_x_2.wav',
                        [(3.95, 6.4)], 'mono'),
    'LMG_L110_1m.wav': (PP + 'L110A2 LMG 5.56mm belt fed (FN MINIMI)/L110A2_LMG_5.56mm_belt_fed_1m_right_MKH8040_1_clean_Single_shots_x_2.wav',
                        [(0.05, 1.7), (3.95, 5.49)], 'mono'),
    'LMG_AKM_Ground.wav': (PP + 'AKM 7.62x39mm/AKM_7.62x39mm_1m_left_on_ground_DPA4062_clean_Triple_shots_x_1.wav', [(0.05, 1.4)], 'mono'),
    'LMG_MG_Field.wav': ('2017/Pole Position - The Warfare Library/warfare_t2_mg_firing_close_projectile_tail_large_field_Telinga_w_MKH8020_or_MKH8060.wav',
                         [(0.0, 1.24), (4.4, 9.0), (23.4, 26.45), (40.15, 41.25), (44.3, 52.0)], 'mono'),
    'LMG_L110_50m.wav': (PP + 'L110A2 LMG 5.56mm belt fed (FN MINIMI)/L110A2_LMG_5.56mm_belt_fed_50m_behind_788_Schoeps_A_clean_Single_shots_x_2.wav',
                         [(0.05, 3.9), (4.03, 5.49)], 'mono'),
    'LMG_L7A2_50m.wav': (PP + 'L7A2 GPMG 7.62x51mm/L7A2_GPMG_7.62x51mm_belt_fed_50m_behind_Schoeps_B_clean_Single_shots_tracer_x_2.wav',
                         [(0.08, 4.0), (4.08, 7.4)], 'mono'),
    'LMG_M2_Warehouse.wav': (AB + 'Audiobeast_Medium_Warehouse_Browning_M2_.50_Machine_Gun_03m_RSM191_MS_Raw_002_Burst_x2.wav',
                             [(19.4, 23.0), (24.6, 30.0)], 'mono'),
    # The Powderkeg launcher and its rocket: an automatic cannon's shot (the launch's thump and report), a designed
    # explosion's whoosh, near explosions (large, without tail, medium with a long tail, a mine, a whoosh's blast), a far
    # explosion and a cannon heard far off through a forest.
    'Launcher_MK44.wav': ('2016/Fascinated Sound - The Gun Locker SFX Pack/Automatic Cannon - MK44 - 03 - Single Shot with Report 03.wav',
                          [(0.2, 6.5)], 'mono'),
    'Launcher_Whoosh.wav': ('2017/Bluezone Corporation - War Zone (Designed Explosions)/Bluezone-BC0214-explosion-whooshe-003.wav',
                            [(0.0, 2.33)], 'mono'),
    'Explosion_Large.wav': (GM + 'Explosion Sound Pack/explosion_large_08.wav', [(0.0, 3.72)], 'mono'),
    'Explosion_NoTail.wav': (GM + 'Explosion Sound Pack/explosion_large_no_tail_03.wav', [(0.0, 1.3)], 'mono'),
    'Explosion_MedTail.wav': (GM + 'Explosion Sound Pack/explosion_med_long_tail_01.wav', [(0.0, 3.1)], 'mono'),
    'Explosion_Mine2.wav': ('2017/Bluezone Corporation - War Zone (Designed Explosions)/Bluezone-BC0214-beep-mine-explosion-002.wav',
                            [(0.35, 6.0)], 'mono'),
    'Explosion_Whoosh.wav': ('2017/Bluezone Corporation - War Zone (Designed Explosions)/Bluezone-BC0214-explosion-whooshe-020.wav',
                             [(2.55, 8.9)], 'mono'),
    'Explosion_Far.wav': (GM + 'Explosion Sound Pack/explosion_far_distant_02.wav', [(0.0, 3.65)], 'mono'),
    'Explosion_Cannon_Far.wav': ('2017/Pole Position - The Warfare Library/warfare_t1b_cannon_firing_forest_distant_MKH8060_2.wav',
                                 [(0.4, 2.7), (2.65, 7.0), (16.15, 17.95), (17.95, 22.0)], 'mono'),
    'Bullet_Cracks.wav': ('2017/Pole Position - The Warfare Library/warfare_t3_mg_whizzes_ricochets_bullet_cracks_M10.wav',
                          [(129.95, 131.35)], 'mono'),
    # Reload and handling foley (the gun feel pass, 2026-10-22: the user's yes to recorded reloads). Every window of an
    # excerpt is the same length, its event (the loudest moment) the same time into it, so recipes/guns_foley.py finds
    # take k at k x (length + GAP) + lead. The G36C's takes alternate magazine in (louder, brighter seat) and out.
    'Foley_G36C_MagIn.wav': (PP + 'Various Gun Foley & Handling/Heckler_&_Koch_G36C_5.56mm_foley_close_up_MKH60_mag_in_&_out.wav',
                             [(p - 0.2, p + 0.3) for p in (1.568, 6.612, 11.556, 16.124, 20.604, 25.376)], 'mono'),
    'Foley_G36C_MagOut.wav': (PP + 'Various Gun Foley & Handling/Heckler_&_Koch_G36C_5.56mm_foley_close_up_MKH60_mag_in_&_out.wav',
                              [(p - 0.5, p + 0.3) for p in (4.214, 9.486, 14.384, 18.340, 23.286, 28.696)], 'mono'),
    # The Saiga's charging handle: two quick racks whole, then single pulls and single releases.
    'Foley_Saiga_Rack.wav': (PP + 'Various Gun Foley & Handling/SAIGA-12_12g_solid_slug_foley_close_up_RSM191_R_cocking.wav',
                             [(0.0, 0.8), (2.2, 3.0)], 'mono'),
    'Foley_Saiga_Pull.wav': (PP + 'Various Gun Foley & Handling/SAIGA-12_12g_solid_slug_foley_close_up_RSM191_R_cocking.wav',
                             [(p - 0.05, p + 0.3) for p in (4.460, 7.846, 10.562, 13.492, 16.866, 19.950)], 'mono'),
    'Foley_Saiga_Release.wav': (PP + 'Various Gun Foley & Handling/SAIGA-12_12g_solid_slug_foley_close_up_RSM191_R_cocking.wav',
                                [(p - 0.15, p + 0.35) for p in (6.194, 9.392, 12.274, 15.382, 18.580, 21.796)], 'mono'),
    # The M29's cylinder swung out (softer) and snapped home (brighter, louder).
    'Foley_M29_Open.wav': (PP + 'Various Gun Foley & Handling/Smith_&_Wesson_M29_.44_Magnum_foley_close_up_MKH416_cylinder_open_&_close.wav',
                           [(p - 0.12, p + 0.2) for p in (0.470, 5.298, 10.574, 22.060, 26.660, 38.780)], 'mono'),
    'Foley_M29_Close.wav': (PP + 'Various Gun Foley & Handling/Smith_&_Wesson_M29_.44_Magnum_foley_close_up_MKH416_cylinder_open_&_close.wav',
                            [(p - 0.06, p + 0.25) for p in (2.556, 7.866, 15.010, 19.690, 28.772, 36.914)], 'mono'),
    # A 12 gauge's shells thumbed into the tube, and its pump racked twice.
    'Foley_TS_ShellLoad.wav': (TS + 'Shell Load_Round Eject.wav', [(2.55, 3.35), (4.05, 4.85)], 'mono'),
    'Foley_TS_Pump.wav': (TS + 'Shotgun_Quick Pump_01.wav', [(0.0, 0.6), (0.98, 1.48)], 'mono'),
    # A round dropped on a board (the revolver's empties landing) and a heavy, distorted body punch (a big round's thump).
    'Foley_Brass_Drop.wav': ('2016/Stuart Duffield - Bullet SFX/Bullet_Drop_Wood_04.wav', [(0.1, 0.72)], 'mono'),
    'Impact_PunchHeavy.wav': (GM + 'Punch Sound Pack/punch_heavy_huge_distorted_01.wav', [(0.0, 0.48)], 'mono'),
    # Wood, dirt and flesh (2026-10-23, the user's yes to one more part of the same bundles): boards struck and dropped,
    # rocks thrown into dirt and gravel, a stab, a splat and a crunch. Each window starts a little before its hit.
    'Impact_WoodHard.wav': (DT + 'Impacts Hard - Short Wobbly Tail 01.wav', [(0.25, 0.63)], 'mono'),
    'Impact_WoodCrack.wav': (DT + 'Impacts Soft - Short, Crack.wav', [(0.06, 0.45)], 'mono'),
    'Impact_WoodPlank.wav': (DT + 'Drop Soft - Single Plank, Drop 02.wav', [(0.14, 0.33), (0.31, 0.55)], 'mono'),
    'Impact_DirtRock.wav': (MW + 'Multiple Rock Hit Dirt_RBDII 07.wav',
                            [(p - 0.03, p + 0.4) for p in (0.645, 1.26, 2.965, 4.69, 6.235, 8.615)], 'mono'),
    'Impact_GravelRock.wav': (MW + 'Hit Rock into Gravel_RBDII 06.wav', [(0.06, 0.6)], 'mono'),
    'Flesh_Stab.wav': (GB + 'Stab_Punch_Hit_136.wav', [(0.02, 0.5)], 'mono'),
    'Flesh_Splat.wav': (GB + 'Splatter_Splat_153.wav', [(0.0, 0.45)], 'mono'),
    'Flesh_Crunch.wav': (GB + 'Crunch_Rip_024.wav', [(0.1, 0.45), (1.75, 2.15)], 'mono'),
}


def build(name, src, windows, channels):
    x, rate = S.read_any(os.path.join(RAW, src))
    parts = []
    for t0, t1 in windows:
        seg = x[..., int(t0 * rate):int(t1 * rate)]
        if channels == 'mono' and seg.ndim == 2:
            seg = seg.mean(axis=0)
        seg = S.resample(seg, rate)
        parts.append(fade(seg, 0.002, 0.02))
        parts.append(np.zeros(parts[-1].shape[:-1] + (int(GAP * SR),)))
    y = np.concatenate(parts[:-1], axis=-1)
    # Resampling a recording that touched full scale can overshoot it a little: scaled back under, never clipped.
    top = float(np.max(np.abs(y)))
    if top > 0.999:
        y = y * (0.999 / top)
    S.write24(os.path.join(OUT, name), y)
    on = S.onsets(y if y.ndim == 1 else y.mean(axis=0))
    print(f'{name:30s} {y.shape[-1] / SR:5.2f} s  {1 if y.ndim == 1 else y.shape[0]} ch  shots at '
          + ' '.join(f'{o / SR:.3f}' for o in on))


def main():
    os.makedirs(OUT, exist_ok=True)
    only = sys.argv[1:]
    for name, (src, windows, channels) in EXCERPTS.items():
        if only and name not in only:
            continue
        build(name, src, windows, channels)


if __name__ == '__main__':
    main()
