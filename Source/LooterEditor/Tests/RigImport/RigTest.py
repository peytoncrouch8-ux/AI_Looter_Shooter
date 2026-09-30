"""Fixture for the Looter.Editor.RigImport test: a small rig that shows where a skeleton's bones, skin and hit zones
land in Unreal. Bones: body at (0, 0, 0.5) m, side at +X (Unreal -Y) and top above it; a box on each; a hit zone per
bone. After changing it, export it again:
  Tools\\models.ps1 -Source Source\\LooterEditor\\Tests\\RigImport -Out Source\\LooterEditor\\Tests\\RigImport -NoImport
"""
import looter_model as lm

grey = lm.material('RigTestGrey', 0x808080)
red = lm.material('RigTestRed', 0xff2020)

rig = lm.armature()
lm.bones(rig, [
    ('body', (0.0, 0.0, 0.5), (0.0, -0.4, 0.5), None),
    ('side', (0.5, 0.0, 0.5), (0.9, 0.0, 0.5), 'body'),
    ('top', (0.0, 0.0, 1.0), (0.0, 0.0, 1.3), 'body'),
])
lm.skin(rig, 'RigTest', {
    'body': [lm.box('BodyBox', (0.6, 0.6, 0.4), (0.0, 0.0, 0.5), material=grey)],
    'side': [lm.box('SideBox', (0.4, 0.2, 0.2), (0.7, 0.0, 0.5), material=red)],
    'top': [lm.box('TopBox', (0.2, 0.2, 0.3), (0.0, 0.0, 1.15), material=red)],
})
lm.hit_sphere(rig, 'body', (0.0, 0.0, 0.5), 0.35)
lm.hit_capsule(rig, 'side', (0.5, 0.0, 0.5), (0.9, 0.0, 0.5), 0.12)
lm.hit_sphere(rig, 'top', (0.0, 0.0, 1.15), 0.2)
