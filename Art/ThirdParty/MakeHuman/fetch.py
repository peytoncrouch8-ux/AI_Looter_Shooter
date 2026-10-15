"""Fetches the MakeHuman assets the character pipeline uses into this folder (approved by the user 2026-10-15).

MakeHuman's assets (base mesh, targets, rigs, weights) are CC0 1.0 (public domain: free for commercial use, no
credit required); see LICENSE.ASSETS.md beside this file, fetched with them. Only the assets are used, never
MakeHuman's code (that is AGPL). Source: github.com/makehumancommunity/makehuman, branch master, makehuman/data.

    python Art/ThirdParty/MakeHuman/fetch.py          # fetch what's missing
    python Art/ThirdParty/MakeHuman/fetch.py --force  # fetch everything again

To use another face modifier, add its pair to MODIFIERS (names from data/modifiers/modeling_modifiers.json:
<group>/<target>-<min>.target and -<max>.target) and run this again.
"""
import os
import sys
import urllib.request

ROOT = "https://raw.githubusercontent.com/makehumancommunity/makehuman/master/"
HERE = os.path.dirname(os.path.abspath(__file__))

FILES = [
	"LICENSE.ASSETS.md",
	"makehuman/data/3dobjs/base.obj",
	"makehuman/data/rigs/default.mhskel",
	"makehuman/data/rigs/default_weights.mhw",
]
for ethnic in ("african", "asian", "caucasian"):
	for gender in ("female", "male"):
		FILES.append(f"makehuman/data/targets/macrodetails/{ethnic}-{gender}-young.target")
		for muscle in ("averagemuscle", "maxmuscle", "minmuscle"):
			for weight in ("minweight", "averageweight", "maxweight"):
				if ethnic == "african":   # the universal targets don't depend on ethnicity: fetch them once
					FILES.append(f"makehuman/data/targets/macrodetails/universal-{gender}-young-{muscle}-{weight}.target")

# Face and neck modifiers: (group, target, min, max).
MODIFIERS = [
	("head", "head-scale-vert", "decr", "incr"), ("head", "head-age", "decr", "incr"),
	("forehead", "forehead-temple", "decr", "incr"), ("forehead", "forehead-trans", "backward", "forward"),
	("eyebrows", "eyebrows-trans", "backward", "forward"), ("eyebrows", "eyebrows-angle", "down", "up"), ("eyebrows", "eyebrows-trans", "down", "up"),
	("chin", "chin-prominent", "decr", "incr"), ("chin", "chin-width", "decr", "incr"), ("chin", "chin-height", "decr", "incr"),
	("chin", "chin-bones", "decr", "incr"), ("chin", "chin-jaw-drop", "decr", "incr"),
	("nose", "nose-scale-horiz", "decr", "incr"), ("nose", "nose-scale-vert", "decr", "incr"), ("nose", "nose-point-width", "decr", "incr"),
	("nose", "nose-hump", "decr", "incr"), ("nose", "nose-nostrils-width", "decr", "incr"), ("nose", "nose-width1", "decr", "incr"),
	("nose", "nose-trans", "backward", "forward"), ("nose", "nose-curve", "concave", "convex"),
	("mouth", "mouth-scale-horiz", "decr", "incr"), ("mouth", "mouth-angles", "down", "up"), ("mouth", "mouth-upperlip-volume", "decr", "incr"),
	("mouth", "mouth-lowerlip-volume", "decr", "incr"), ("mouth", "mouth-cupidsbow", "decr", "incr"),
	("neck", "neck-scale-horiz", "decr", "incr"), ("neck", "neck-scale-depth", "decr", "incr"),
	("torso", "torso-vshape", "decr", "incr"), ("torso", "torso-scale-horiz", "decr", "incr"),
]
for side in ("l", "r"):
	MODIFIERS += [
		("cheek", f"{side}-cheek-bones", "decr", "incr"), ("cheek", f"{side}-cheek-volume", "decr", "incr"),
		("cheek", f"{side}-cheek-inner", "decr", "incr"),
		("eyes", f"{side}-eye-scale", "decr", "incr"), ("eyes", f"{side}-eye-bag", "decr", "incr"),
		("eyes", f"{side}-eye-height2", "decr", "incr"), ("eyes", f"{side}-eye-eyefold", "down", "up"),
		("eyes", f"{side}-eye-corner1", "down", "up"),
		("ears", f"{side}-ear-scale", "decr", "incr"), ("ears", f"{side}-ear-flap", "decr", "incr"),
	]
for group, target, lo, hi in MODIFIERS:
	for end in (lo, hi):
		FILES.append(f"makehuman/data/targets/{group}/{target}-{end}.target")
# One-sided head shapes (a single target each).
for shape in ("oval", "square", "invertedtriangular", "diamond", "rectangular"):
	FILES.append(f"makehuman/data/targets/head/head-{shape}.target")


def main():
	force = "--force" in sys.argv
	got = 0
	for rel in dict.fromkeys(FILES):
		dest = os.path.join(HERE, rel.replace("makehuman/data/", ""))
		if os.path.exists(dest) and not force:
			continue
		os.makedirs(os.path.dirname(dest), exist_ok=True)
		try:
			with urllib.request.urlopen(ROOT + rel, timeout=60) as r:
				data = r.read()
		except Exception as e:
			print("missing:", rel, e)
			continue
		with open(dest, "wb") as f:
			f.write(data)
		got += 1
	print(f"fetched {got} files into {HERE}")


if __name__ == "__main__":
	main()
