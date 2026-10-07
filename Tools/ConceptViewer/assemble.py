"""Assembles the concept viewer page: styles and markup, the island data, the import map and one module script.

Reads the page sources from Tools/ConceptViewer/web and the exported models from the build folder (default
Saved/ConceptViewer/web). Writes into the build folder: index.html (the page to publish), test.html (the same page with
a document skeleton, for testing locally) and a .json copy of every binary file. Writes publish_files.json beside the
build folder, listing the files to publish next to the page.
  python3 Tools/ConceptViewer/assemble.py [build folder]
"""
import base64
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, '..', '..'))
SRC = os.path.join(HERE, 'web')
W = os.path.abspath(sys.argv[1]) if len(sys.argv) > 1 else os.path.join(REPO, 'Saved', 'ConceptViewer', 'web')
SCRIPTS = ['engine.js', 'kit.js', 'concepts.js', 'app.js', 'ui.js']

layout = json.load(open(os.path.join(REPO, 'Art/Levels/TutorialIsland/layout.json')))
computed = json.load(open(os.path.join(REPO, 'Art/Levels/TutorialIsland/layout_computed.json')))
manifest = json.load(open(os.path.join(W, 'manifest.json')))
manifest['models'].pop('root', None)


# The artifact host serves JSON but not .glb or .bin, so each binary file ships as a base64 string in a .json beside it.
def packed(rel):
    out = os.path.splitext(rel)[0] + '.json'
    with open(os.path.join(W, rel), 'rb') as f:
        data = base64.b64encode(f.read()).decode('ascii')
    with open(os.path.join(W, out), 'w') as f:
        f.write('"' + data + '"')
    return out


for entry in manifest['models'].values():
    entry['file'] = packed(entry['file'])
for key in ('terrain', 'underside', 'water', 'heights'):
    manifest['terrain'][key]['file'] = packed(manifest['terrain'][key]['file'])

keep = ('cliffs', 'bridge', 'waterfall', 'pond', 'creek', 'ramp', 'orchardRows', 'footprints', 'placements')
data = dict(layout=layout, computed={k: computed[k] for k in keep if k in computed}, manifest=manifest)
code = '\n'.join(open(os.path.join(SRC, f)).read() for f in SCRIPTS)
three = 'https://cdn.jsdelivr.net/npm/three@0.160.0/'
html = ''.join([
    open(os.path.join(SRC, 'ui.html')).read(),
    '<script>window.__DATA__ = ' + json.dumps(data, separators=(',', ':')) + ';</script>\n',
    '<script type="importmap">{"imports":{"three":"' + three + 'build/three.module.js","three/addons/":"' + three
    + 'examples/jsm/"}}</script>\n',
    '<script type="module">\n' + code + '\n</script>\n'])
# index.html is what gets published (the artifact service adds the document skeleton); test.html carries the skeleton.
open(os.path.join(W, 'index.html'), 'w').write(html)
open(os.path.join(W, 'test.html'), 'w').write(
    '<!doctype html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, '
    'initial-scale=1, viewport-fit=cover"></head><body style="margin:0">' + html + '</body></html>')
files = [e['file'] for e in manifest['models'].values()] + \
    [manifest['terrain'][k]['file'] for k in ('terrain', 'underside', 'water', 'heights')]
json.dump([{'path': f} for f in sorted(files)], open(os.path.join(os.path.dirname(W), 'publish_files.json'), 'w'))
total = len(html) + sum(os.path.getsize(os.path.join(W, f)) for f in files)
print(f'index.html {len(html) / 1024:.0f} KB; with {len(files)} model and height files {total / 1024 / 1024:.1f} MB')
