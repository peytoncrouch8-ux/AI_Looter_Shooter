"""Per-person numbers for parts_people.py (the story people's Tripo bodies on their script rigs). Raw space (see
parts_people_src): meters, front -Y, feet on z 0, +X the person's left; joints measured from
Saved/ArtPreviews/TripoPeople/<Name>_front/_side.png (Docs/Work/2026-10-31-Wardens/tripo_people/glb_views.py)."""

PEOPLE = {
    'Abel': dict(floor=0.925, cut=0.30, tris=19000, fists='lr', grips={'l': 'lantern', 'r': 'gun'}, joints=dict(
        shoulder_l=(0.18, 0.0, 1.46), elbow_l=(0.44, -0.03, 1.45), wrist_l=(0.70, -0.07, 1.415),
        knuckles_l=(0.80, -0.09, 1.41), hips=(0.0, -0.01, 0.95), neck=(0.0, 0.0, 1.50), head=(0.0, -0.02, 1.60),
        hinge=(0.0, -0.03, 1.655), chin=(0.0, -0.13, 1.565), crown=(0.0, -0.04, 1.845), face=(0.0, -0.10, 1.66),
        knee=(0.0, -0.03, 0.50)),
        mouth=dict(x=0.003, z=1.627, y=-0.145, half_width=0.036), lower='skirts',
        zones=('#D9D6CC', '#2B2A2E', '#C2A873'), skin='#B8BCC4'),
    'Foreman': dict(floor=0.925, cut=0.38, tris=18000, fists='lr', grips={'r': 'pick'}, lower='skirts', joints=dict(
        shoulder_l=(0.19, 0.06, 1.44), elbow_l=(0.43, 0.04, 1.42), wrist_l=(0.67, 0.02, 1.40),
        knuckles_l=(0.80, -0.01, 1.39), hips=(0.0, 0.0, 0.95), knee=(0.0, 0.0, 0.50), neck=(0.0, 0.02, 1.50),
        head=(0.0, 0.0, 1.585), hinge=(0.0, -0.02, 1.63), chin=(0.0, -0.12, 1.556), crown=(0.0, 0.0, 1.85),
        face=(0.0, -0.10, 1.65)),
        mouth=dict(x=0.0, z=1.611, y=-0.145, half_width=0.03),
        zones=('#D8D0B8', '#5A3A2A', '#161515'), skin='#C9C3C8', prop_scale={'ForemanSatchel': (1.2, 1.25)}),
    'Amos': dict(floor=0.9, cut=0.47, tris=15000, fists='r', grips={'r': 'fork'}, lower='thighs', joints=dict(
        shoulder_l=(0.18, 0.0, 1.38), elbow_l=(0.45, -0.03, 1.345), wrist_l=(0.70, -0.06, 1.32),
        knuckles_l=(0.79, -0.07, 1.31), hips=(0.0, -0.04, 0.92), knee=(0.0, -0.04, 0.48), neck=(0.0, 0.0, 1.47),
        head=(0.0, 0.0, 1.55), hinge=(0.0, -0.03, 1.61), chin=(0.0, -0.13, 1.503), crown=(0.0, -0.03, 1.80),
        face=(0.0, -0.11, 1.60)),
        mouth=dict(x=0.003, z=1.5875, y=-0.15, half_width=0.034),
        zones=('#D9CFB0', '#7A4B2E', '#4A3020'), skin='#8E9CC0', grade=[(200, 250, 0.45)]),
    'Hiram': dict(floor=0.925, cut=0.425, tris=15000, fists='', lower='thighs', seated=True, joints=dict(
        shoulder_l=(0.18, 0.05, 1.43), elbow_l=(0.42, 0.02, 1.38), wrist_l=(0.647, -0.01, 1.33),
        knuckles_l=(0.73, -0.05, 1.30), hips=(0.0, 0.02, 0.95), knee=(0.0, 0.02, 0.50), neck=(0.0, 0.03, 1.52),
        head=(0.0, 0.0, 1.585), hinge=(0.0, -0.04, 1.64), chin=(0.0, -0.13, 1.53), crown=(0.0, -0.02, 1.85),
        face=(0.0, -0.11, 1.66)),
        mouth=dict(x=0.003, z=1.634, y=-0.145, half_width=0.026),
        zones=('#B8C0C8', '#B08850', '#4A3426'), skin='#A9AEBF'),
    'Thaddeus': dict(floor=0.9, cut=0.395, tris=14000, fists='', lower='thighs', keep_hand_cloth=True, joints=dict(
        shoulder_l=(0.17, 0.08, 1.44), elbow_l=(0.40, 0.03, 1.27), wrist_l=(0.56, -0.08, 1.14),
        knuckles_l=(0.61, -0.14, 1.08), hips=(0.0, 0.02, 0.93), knee=(0.0, 0.0, 0.50), neck=(0.0, 0.05, 1.49),
        head=(0.0, 0.04, 1.555), hinge=(0.0, 0.0, 1.615), chin=(0.0, -0.07, 1.551), crown=(0.0, 0.02, 1.80),
        face=(0.0, -0.05, 1.62)),
        mouth=dict(x=0.0, z=1.5875, y=-0.08, half_width=0.03),
        zones=('#D8D4C4', '#2B2A28', '#B06A30'), skin='#A8ACB8', grade=[(15, 40, 0.7)]),
    'DeadMiner': dict(floor=0.925, cut=0.55, tris=12000, fists='', grips={'r': 'pick'}, lower='pelvis', joints=dict(
        shoulder_l=(0.18, 0.05, 1.38), elbow_l=(0.43, 0.03, 1.10), wrist_l=(0.66, -0.08, 0.93),
        knuckles_l=(0.73, -0.15, 0.86), hips=(0.0, 0.0, 0.79), neck=(0.0, 0.0, 1.47), head=(0.0, -0.03, 1.54),
        hinge=(0.0, -0.09, 1.60), chin=(0.0, -0.17, 1.51), crown=(0.0, -0.03, 1.85), face=(0.0, -0.15, 1.62)),
        mouth=dict(x=0.0, z=1.573, y=-0.19, half_width=0.035),
        zones=('#7A3A30', '#B89060', '#2E4F9A'), skin='#8E98B8', grade=[(200, 250, 0.5)]),
}
