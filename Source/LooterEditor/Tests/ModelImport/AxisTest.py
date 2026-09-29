"""Fixture for the Looter.Editor.ModelImport test: an asymmetric block that shows where Blender's axes land in Unreal.
Bounds x [-1, 1.2], y [-1.1, 0.5], z [0, 0.8] m, one box hull, and two sockets. After changing it, export it again:
  Tools\\models.ps1 -Source Source\\LooterEditor\\Tests\\ModelImport -Out Source\\LooterEditor\\Tests\\ModelImport -NoImport
"""
import looter_model as lm

grey = lm.material('AxisTestGrey', 0x808080)
red = lm.material('AxisTestRed', 0xff2020)
body = lm.box('AxisTest', size=(2.0, 1.0, 0.5), center=(0.0, 0.0, 0.25), material=grey)
lm.box('PlusX', size=(0.2, 0.2, 0.2), center=(1.1, 0.0, 0.25), material=red, parent=body)
lm.box('Front', size=(0.2, 0.6, 0.2), center=(0.0, -0.8, 0.25), material=grey, parent=body)
lm.box('Top', size=(0.2, 0.2, 0.3), center=(0.5, 0.0, 0.65), material=red, parent=body)
lm.hull_box(body, size=(2.0, 1.0, 0.5), center=(0.0, 0.0, 0.25))
lm.socket(body, 'PlusX', location=(1.2, 0.0, 0.25))
# Turned 90 degrees counterclockwise seen from above: its front (-Y) now points at Blender +X, which is Unreal -Y (yaw -90).
lm.socket(body, 'TurnLeft', location=(0.0, 0.0, 0.5), rotation=(0.0, 0.0, 90.0))
