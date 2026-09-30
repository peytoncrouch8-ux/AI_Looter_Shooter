"""Fixture for the Looter.Editor.TexturedImport test: the textured art style's import path.
- TexturedBlock: a box whose material names the World master and the TexturedTest set (the three PNGs beside this file),
  with a tint and a UV scale.
- LodBlock: Nanite off, with LODs at 50% (screen size 0.4).
- GhostBlock: no collision at all.
After changing it, export it again:
  Tools\\models.ps1 -Source Source\\LooterEditor\\Tests\\TexturedImport -Out Source\\LooterEditor\\Tests\\TexturedImport -NoImport
"""
import os
import sys

import bpy
import looter_model as lm

# This script's folder (Blender runs it with --python <file>).
HERE = os.path.dirname(os.path.abspath(sys.argv[sys.argv.index('--python') + 1]))


def textured(name, set_name):
    material = bpy.data.materials.new(name)
    material.use_nodes = True
    nodes = material.node_tree.nodes
    for suffix in ('_BC', '_N', '_ORM'):
        image = bpy.data.images.load(os.path.join(HERE, f'T_{set_name}{suffix}.png'))
        node = nodes.new('ShaderNodeTexImage')
        node.image = image
    material['Master'] = 'World'
    material['TextureSet'] = set_name
    material['Tint'] = '#FF8040'
    material['UVScale'] = 2.0
    return material


look = textured('TexturedTestLook', 'TexturedTest')
block = lm.box('TexturedBlock', size=(1.0, 1.0, 1.0), center=(0.0, 0.0, 0.5), material=look)
lm.hull_box(block, size=(1.0, 1.0, 1.0), center=(0.0, 0.0, 0.5))

lod = lm.cylinder('LodBlock', radius=0.5, height=1.0, center=(0.0, 0.0, 0.5), segments=48, material=look)
lod['Nanite'] = 0
lod['LODs'] = '50'
lod['LODScreens'] = '0.4'

ghost = lm.box('GhostBlock', size=(1.0, 1.0, 1.0), center=(0.0, 0.0, 0.5), material=look)
ghost['Collision'] = 'None'
