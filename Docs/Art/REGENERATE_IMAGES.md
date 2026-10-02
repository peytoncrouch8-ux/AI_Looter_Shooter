# This branch holds the art-style work without its images

The cloud session that made the style exploration could not upload Git LFS objects (its network policy denies
`lfs.github.com`), so this branch carries everything except the PNGs: the style document and transition plan
(`ScreenPrintWash.md`), the exploration notes (`StyleExploration/README.md`), the plan and doc pointers, and the
scripts that made the images (`Tools/Blender/style_render.py`, `style_compose.py`, `style_plugins/`). The full branch
with the images is `claude/art-style-exploration`; the user also received every image in the session's chat.

Missing here: `StyleTarget_ScreenPrintWash.png`, `StyleExploration/*.png` and `StyleExploration/hybrids/*.png`.

To make them again (about four minutes; Blender 4.5 or the `bpy` 4.5 module, plus numpy, opencv-contrib-python-headless
and pillow in that Python):

    python Tools/Blender/style_render.py Art/Models/Buildings/Cottage.py Saved/StyleExplore/Cottage
    python Tools/Blender/style_compose.py Saved/StyleExplore/Cottage Docs/Art/StyleExploration
    python Tools/Blender/style_compose.py Saved/StyleExplore/Cottage Docs/Art/StyleExploration/hybrids --no-builtin --plugins Tools/Blender/style_plugins/hybrid_storybook_ink.py,Tools/Blender/style_plugins/screen_print_wash.py,Tools/Blender/style_plugins/hybrid_moebius_watercolor.py,Tools/Blender/style_plugins/hybrid_inked_wash.py,Tools/Blender/style_plugins/hybrid_gouache_inked_edges.py,Tools/Blender/style_plugins/hybrid_pastel_flat_gouache.py
    copy Docs\Art\StyleExploration\hybrids\10_screen_print_wash.png Docs\Art\StyleTarget_ScreenPrintWash.png

The hybrids' sheet of all six is `contact_sheet.png` in `hybrids/`; the earlier three-only sheet was the same tiles.
The same scripts gave byte-identical images on repeated runs in one environment; another Blender or OpenCV build
renders the same picture with small pixel differences. Delete this file once the images are in.
