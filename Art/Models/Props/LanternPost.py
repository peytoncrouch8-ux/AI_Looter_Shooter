"""A lantern post for paths and camps: a wooden post with an iron arm, and a warm lantern hanging toward the front.
A scripted model (see Art/README.md); the post's foot is the origin and the lantern hangs toward the front (-Y)."""
import looter_model as lm

wood = lm.material('LanternWood', 0x7a5634, GradHeight=2.2, GradDark=0.35, Variation=0.18)
iron = lm.material('LanternIron', 0x34373d, Variation=0.06)
# Lit glass is an opaque, emissive surface: Nanite can't draw the additive Glow kind (that's for light beams).
glow = lm.material('LanternGlow', 0xffc56e, Glow=5.0, Variation=0.04)

post = lm.box('LanternPost', size=(0.16, 0.16, 2.3), center=(0.0, 0.0, 1.15), material=wood, bevel=0.02)
lm.box('Foot', size=(0.36, 0.36, 0.14), center=(0.0, 0.0, 0.07), material=wood, parent=post, bevel=0.03)
lm.box('Arm', size=(0.07, 0.62, 0.07), center=(0.0, -0.27, 2.16), material=iron, parent=post, bevel=0.01)
lm.box('Brace', size=(0.05, 0.05, 0.36), center=(0.0, -0.13, 2.0), material=iron, parent=post)

# The lantern hangs from the end of the arm.
hang = (0.0, -0.52)
lm.box('Hook', size=(0.03, 0.03, 0.12), center=(*hang, 2.07), material=iron, parent=post)
lm.box('Cap', size=(0.28, 0.28, 0.06), center=(*hang, 1.98), material=iron, parent=post, bevel=0.015)
lm.box('Glass', size=(0.2, 0.2, 0.26), center=(*hang, 1.82), material=glow, parent=post)
lm.box('Base', size=(0.28, 0.28, 0.05), center=(*hang, 1.665), material=iron, parent=post, bevel=0.012)
for dx in (-0.115, 0.115):
    for dy in (-0.115, 0.115):
        lm.box('Bar', size=(0.03, 0.03, 0.3), center=(hang[0] + dx, hang[1] + dy, 1.82), material=iron, parent=post)

lm.hull_box(post, size=(0.36, 0.36, 2.3), center=(0.0, 0.0, 1.15))
lm.hull_box(post, size=(0.3, 0.3, 0.46), center=(*hang, 1.84))
lm.socket(post, 'Light', location=(*hang, 1.82))
