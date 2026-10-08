"""Builds the Style Lab site, ready to publish: Saved/StyleLab/site/

    python3 Tools/StyleLab/assemble.py [--export Saved/StyleLab/export] [--out Saved/StyleLab/site] [--b64-glb]

Copies web/ (index.html at the root, engine/, styles/, hud/) and the export (as data/: manifest.json, scene.json,
models/, tex/, terrain/). styles/index.js gets the list of style files on disk. The heights file ships as a base64
.json (hosts that serve JSON may refuse .bin); --b64-glb does the same for the model packs (they grow by a third, so
packs must then be under about 10.5 MB). Checks the limits (each file <= 14 MB, at most 240 files) and writes
publish_files.json: every file but index.html, as {"path": ...} relative to the site folder. Also writes test.html,
the page with a document skeleton, for opening locally (index.html is a fragment, as the artifact host wants it).
"""
import argparse
import base64
import json
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, '..', '..'))
WEB = os.path.join(HERE, 'web')
WEB_TYPES = {'.html', '.js', '.mjs', '.css', '.json', '.webp', '.png', '.jpg', '.svg', '.woff2', '.glb', '.bin'}
MAX_FILE = 14 * 1024 * 1024
MAX_FILES = 240


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--export', default=os.path.join(REPO, 'Saved', 'StyleLab', 'export'))
    ap.add_argument('--out', default=os.path.join(REPO, 'Saved', 'StyleLab', 'site'))
    ap.add_argument('--b64-glb', action='store_true')
    a = ap.parse_args()
    out = os.path.abspath(a.out)
    if not os.path.exists(os.path.join(a.export, 'manifest.json')):
        sys.exit(f'no export at {a.export} (run Tools/StyleLab/export/export_all.sh)')
    if os.path.isdir(out):
        shutil.rmtree(out)
    os.makedirs(out)

    # the page and its modules
    for root, dirs, files in os.walk(WEB):
        rel_root = os.path.relpath(root, WEB)
        for f in files:
            ext = os.path.splitext(f)[1].lower()
            if ext not in WEB_TYPES or f == 'demo.html':
                continue
            src = os.path.join(root, f)
            dst = os.path.join(out, rel_root, f) if rel_root != '.' else os.path.join(out, f)
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            shutil.copyfile(src, dst)
    styles_dir = os.path.join(out, 'styles')
    style_files = sorted(f for f in os.listdir(styles_dir) if re.match(r'^\d\d_[\w-]+\.js$', f))
    idx = os.path.join(styles_dir, 'index.js')
    text = open(idx, encoding='utf-8').read()
    text = re.sub(r'/\*FILES\*/[\s\S]*?/\*END\*/', '/*FILES*/' + ', '.join(f"'{f}'" for f in style_files) + '/*END*/', text)
    open(idx, 'w', encoding='utf-8').write(text)

    # the export as data/
    data = os.path.join(out, 'data')
    manifest = json.load(open(os.path.join(a.export, 'manifest.json')))
    scene_src = os.path.join(a.export, 'scene.json')

    def copy(rel):
        src = os.path.join(a.export, rel)
        if not os.path.exists(src):
            print('  missing in the export:', rel)
            return rel
        dst = os.path.join(data, rel)
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        shutil.copyfile(src, dst)
        return rel

    def b64(rel):
        src = os.path.join(a.export, rel)
        if not os.path.exists(src):
            print('  missing in the export:', rel)
            return rel
        new = os.path.splitext(rel)[0] + '.json'
        dst = os.path.join(data, new)
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        with open(src, 'rb') as f, open(dst, 'w') as g:
            g.write('"' + base64.b64encode(f.read()).decode('ascii') + '"')
        return new

    packer = b64 if a.b64_glb else copy
    manifest['packs'] = [packer(p) for p in manifest.get('packs', [])]
    t = manifest.get('terrain', {})
    if t.get('near'):
        t['near'] = packer(t['near'])
    if t.get('far'):
        t['far'] = [packer(f) for f in ([t['far']] if isinstance(t['far'], str) else t['far'])]
    if t.get('heights'):
        t['heights']['file'] = b64(t['heights']['file'])
    # every texture the manifest names (sets and terrain maps), and anything else in tex/ the scene's rules read
    texs = set()
    for s in manifest.get('textureSets', {}).values():
        for k in ('bc', 'n', 'orm'):
            if s.get(k):
                texs.add(s[k])
    for k, v in t.items():
        if isinstance(v, str) and re.match(r'^tex/[\w.-]+$', v):
            texs.add(v)
    scene = json.load(open(scene_src)) if os.path.exists(scene_src) else {}
    for r in scene.get('scatter', []):
        if r.get('maskTex'):
            texs.add(r['maskTex'])
    for rel in sorted(texs):
        copy(rel)
    os.makedirs(data, exist_ok=True)
    json.dump(manifest, open(os.path.join(data, 'manifest.json'), 'w'), separators=(',', ':'))
    json.dump(scene, open(os.path.join(data, 'scene.json'), 'w'), separators=(',', ':'))

    # test.html: the fragment in a document
    frag = open(os.path.join(out, 'index.html'), encoding='utf-8').read()
    open(os.path.join(out, 'test.html'), 'w', encoding='utf-8').write(
        '<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, '
        'initial-scale=1, viewport-fit=cover"></head><body>' + frag + '</body></html>')

    # limits and the publish list
    files, total, big = [], 0, []
    for root, dirs, fs in os.walk(out):
        for f in fs:
            p = os.path.join(root, f)
            rel = os.path.relpath(p, out).replace(os.sep, '/')
            size = os.path.getsize(p)
            total += size
            if size > MAX_FILE:
                big.append((rel, size))
            if rel not in ('index.html', 'test.html', 'publish_files.json'):
                files.append(rel)
    files.sort()
    json.dump([{'path': f} for f in files], open(os.path.join(out, 'publish_files.json'), 'w'), indent=0)
    print(f'site: {out}')
    print(f'  {len(files) + 1} files to publish (index.html + {len(files)}), {total / 1024 / 1024:.1f} MB in all; styles: {", ".join(style_files)}')
    ok = True
    for rel, size in big:
        print(f'  TOO BIG: {rel} {size / 1024 / 1024:.1f} MB (limit 14 MB)')
        ok = False
    if len(files) + 1 > MAX_FILES:
        print(f'  TOO MANY FILES: {len(files) + 1} (limit about {MAX_FILES})')
        ok = False
    print('  limits: ' + ('OK' if ok else 'FAILED'))
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
