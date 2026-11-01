"""Tripo image-to-model and multiview-to-model (API v3) for gun parts made from rendered views of our own scripted parts
(lane TRIPO-PISTOLS, 2026-11-01: the user approved the Drover's part sets). Uses Tools/tripo.py's key, call(), wait(),
download() and balance(); tripo.py itself is left alone (other lanes use it).

Usage (UE's bundled python works: no packages needed):
  python Tools/tripo_image.py spent [<Family>]                 credits this family's tasks used (its task.json files)
  python Tools/tripo_image.py views <Family> <Part> [--faces N] [--redo] [--quality standard|detailed]
        multiview-to-model from Art/Concepts/Weapons/<Family>/<Part>/upload/{front,left,back,right}.png
  python Tools/tripo_image.py image <Family> <Part> <image.png> [--faces N] [--redo]
        image-to-model from one picture
  python Tools/tripo_image.py repaint <Family> <Part> [--redo]
        new textures for the made model, guided by its own four views again (texture_prompt.images)
  python Tools/tripo_image.py batch <Family> <cap> [<Part> ...]
        views for each part not made yet (all the family's parts in PARTS when none are named), stopping before the
        family's credits would pass cap
Results land in Art/Tripo/<Family>/<Part>/: task.json (every task id, its status and credits; earlier attempts kept
under "attempts"), <Part>.glb (the textured model), preview.png (Tripo's render). A finished task recorded in task.json
is never paid for again (--redo makes a new one and keeps the old under "attempts").

Every family has a credit cap (CAPS): a task is refused when the family's recorded credits plus the task's estimate
would pass it, so lanes spending at the same time can't overrun theirs.
"""

import json
import mimetypes
import os
import sys
import urllib.error
import urllib.request
import uuid

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import tripo  # noqa: E402  (its key(), call(), wait(), download(), balance(), BASE, ROOT)

CONCEPTS = os.path.join(tripo.ROOT, 'Art', 'Concepts', 'Weapons')
OUT = os.path.join(tripo.ROOT, 'Art', 'Tripo')
MODEL = 'v3.1-20260211'
TEXTURE_MODEL = 'v3.0-20250812'
QUALITY = 'standard'   # 'detailed' costs 40 a multiview model (2026-11-01, Body_Blued); our maps stop at 2048 anyway
ESTIMATE = {'model': 40.0, 'repaint': 20.0}   # credits held back per task before Tripo says (it charges on success)
CAPS = {'Drover': 750.0}                       # the user's approval per family (2026-11-01: Drover 750, retries included)
VIEWS = ('front', 'left', 'back', 'right')     # the API's order

# The parts each family's batch makes, by set (Tools/Blender/drover_views.py renders their views), with a face limit
# sized to the part: a revolver of five parts lands near the AR's 10k-a-part budget for its whole gun's six.
PARTS = {
    'Drover': {
        'Body_Blued': 12000, 'Barrel_Service': 6000, 'Cylinder_Fluted6': 8000, 'Sight_Notch': 3000,
        'Grip_Plowhandle': 8000,
        'Body_Ironclad': 12000, 'Barrel_Lugged': 6000, 'Cylinder_Bigbore5': 8000, 'Sight_Glass': 6000,
        'Grip_Coffin': 8000,
        'Body_Scrapper': 12000, 'Barrel_Comped': 6000, 'Cylinder_Plain6': 8000, 'Sight_Peep': 3000,
        'Grip_Wrapped': 8000,
        'Body_Ranger': 12000, 'Barrel_Vented': 6000, 'Cylinder_Moon6': 8000, 'Sight_Pip': 4000,
        'Grip_Target': 8000,
    },
}


def upload(path):
    """Uploads an image; returns its file token."""
    boundary = uuid.uuid4().hex
    kind = mimetypes.guess_type(path)[0] or 'image/png'
    with open(path, 'rb') as f:
        data = f.read()
    body = (f'--{boundary}\r\nContent-Disposition: form-data; name="file"; filename="{os.path.basename(path)}"\r\n'
            f'Content-Type: {kind}\r\n\r\n').encode() + data + f'\r\n--{boundary}--\r\n'.encode()
    request = urllib.request.Request(tripo.BASE + '/files', data=body, method='POST', headers={
        'Authorization': 'Bearer ' + tripo.key(), 'Content-Type': f'multipart/form-data; boundary={boundary}'})
    try:
        with urllib.request.urlopen(request, timeout=120) as response:
            reply = json.loads(response.read().decode())
    except urllib.error.HTTPError as error:
        sys.exit(f'Tripo upload {path}: HTTP {error.code}: {error.read().decode(errors="replace")[:400]}')
    if reply.get('code') != 0:
        sys.exit(f'Tripo upload {path}: {json.dumps(reply)[:400]}')
    data = reply.get('data', {})
    return data.get('file_token') or data.get('image_token')


def folder(family, part):
    path = os.path.join(OUT, family, part)
    os.makedirs(path, exist_ok=True)
    return path


def load(family, part):
    path = os.path.join(folder(family, part), 'task.json')
    return json.load(open(path)) if os.path.exists(path) else {}


def save(family, part, record):
    json.dump(record, open(os.path.join(folder(family, part), 'task.json'), 'w'), indent=2)


def spent(family):
    """Credits the family's recorded tasks used (finished ones; earlier attempts too)."""
    total = 0.0
    root = os.path.join(OUT, family)
    if not os.path.isdir(root):
        return 0.0
    for part in os.listdir(root):
        path = os.path.join(root, part, 'task.json')
        if not os.path.exists(path):
            continue
        record = json.load(open(path))
        tasks = [v for k, v in record.items() if k != 'attempts' and isinstance(v, dict)] + record.get('attempts', [])
        total += sum(float(t.get('credits') or 0) for t in tasks)
    return total


def check_cap(family, kind):
    cap = CAPS.get(family)
    used = spent(family)
    if cap is not None and used + ESTIMATE[kind] > cap:
        sys.exit(f'{family}: {used} credits used of its cap {cap}: a {kind} task (about {ESTIMATE[kind]}) would pass it.')
    return used


def view_paths(family, part):
    base = os.path.join(CONCEPTS, family, part, 'upload')
    paths = {v: os.path.join(base, f'{v}.png') for v in VIEWS}
    missing = [p for p in paths.values() if not os.path.exists(p)]
    if missing:
        sys.exit(f'{family} {part}: no views ({missing[0]}); render them first')
    return paths


def run(family, part, kind, body, endpoint, redo=False):
    """Starts the task (unless a finished one is recorded), waits, downloads; returns the credits it used."""
    record = load(family, part)
    slot = 'model' if kind == 'model' else 'paint'
    done = record.get(slot, {}).get('status') == 'success'
    if done and not redo:
        print(f'{family} {part}: {slot} already made ({record[slot]["task_id"]})', flush=True)
        return 0.0
    if record.get(slot, {}).get('status') == 'running' and not redo:
        state = tripo.call('GET', f'/tasks/{record[slot]["task_id"]}').get('status')
        if state in ('failed', 'cancelled', 'banned', 'expired', 'unknown'):
            record[slot]['status'] = state   # not charged (Tripo returns a failed task's credits): start again
    if record.get(slot, {}).get('status') == 'running' and not redo:
        task_id = record[slot]['task_id']   # a crash mid-wait: pick the task up again rather than pay twice
        print(f'{family} {part}: resuming {slot} task {task_id}', flush=True)
    else:
        check_cap(family, kind)
        if record.get(slot):
            record.setdefault('attempts', []).append(record[slot])
        task_id = tripo.call('POST', endpoint, body)['task_id']
        record[slot] = dict(task_id=task_id, status='running', kind=endpoint)
        save(family, part, record)
    result = tripo.wait(task_id, f'{part} {slot}')
    credits = float(result.get('credits_consumed') or 0)
    record[slot] = dict(task_id=task_id, status='success', credits=credits, kind=endpoint)
    save(family, part, record)
    out = folder(family, part)
    output = result.get('output', {})
    glb = os.path.join(out, f'{part}.glb')
    if slot == 'paint' and os.path.exists(glb):
        os.replace(glb, os.path.join(out, f'{part}_first.glb'))
    tripo.download(output.get('pbr_model_url') or output['model_url'], glb)
    if output.get('rendered_image_url'):
        tripo.download(output['rendered_image_url'],
                       os.path.join(out, 'preview.png' if slot == 'model' else 'preview_painted.png'))
    print(f'{family} {part}: {credits} credits; the family has used {spent(family)}', flush=True)
    return credits


def make_views(family, part, faces=None, redo=False, quality=None):
    paths = view_paths(family, part)
    tokens = [{v: upload(paths[v])} for v in VIEWS]
    body = dict(inputs=tokens, model=MODEL, texture=True, pbr=True, texture_quality=quality or QUALITY,
                geometry_quality='standard', texture_alignment='original_image', orientation='align_image',
                face_limit=faces or PARTS.get(family, {}).get(part, 10000))
    return run(family, part, 'model', body, '/generation/multiview-to-model', redo)


def make_image(family, part, image, faces=None, redo=False):
    body = dict(input=upload(image), model=MODEL, texture=True, pbr=True, texture_quality='detailed',
                geometry_quality='standard', texture_alignment='original_image', orientation='align_image',
                face_limit=faces or PARTS.get(family, {}).get(part, 10000))
    return run(family, part, 'model', body, '/generation/image-to-model', redo)


def repaint(family, part, redo=False):
    record = load(family, part)
    if record.get('model', {}).get('status') != 'success':
        sys.exit(f'{family} {part}: no model to repaint')
    paths = view_paths(family, part)
    images = [{v: upload(paths[v])} for v in VIEWS]
    body = dict(input=record['model']['task_id'], model=TEXTURE_MODEL, texture_prompt=dict(images=images), pbr=True,
                texture_quality='detailed', texture_alignment='original_image')
    return run(family, part, 'repaint', body, '/models/texture', redo)


def batch(family, cap, parts):
    for part in parts or list(PARTS[family]):
        if load(family, part).get('model', {}).get('status') == 'success':
            continue
        if spent(family) + ESTIMATE['model'] > cap:
            print(f'BATCH stop before {part}: {spent(family)} credits used, cap {cap}', flush=True)
            return
        try:
            make_views(family, part)
        except SystemExit as stop:
            print(f'BATCH {part} failed: {stop}', flush=True)
    print(f'BATCH done: the family has used {spent(family)} credits', flush=True)


def flag(args, name, cast=str):
    return cast(args[args.index(name) + 1]) if name in args else None


if __name__ == '__main__':
    args = sys.argv[1:]
    if not args:
        print(__doc__)
    elif args[0] == 'spent':
        for fam in ([args[1]] if len(args) > 1 else sorted(PARTS)):
            print(f'{fam}: {spent(fam)} credits used (cap {CAPS.get(fam)})')
    elif args[0] == 'views' and len(args) >= 3:
        make_views(args[1], args[2], flag(args, '--faces', int), '--redo' in args, flag(args, '--quality'))
    elif args[0] == 'image' and len(args) >= 4:
        make_image(args[1], args[2], args[3], flag(args, '--faces', int), '--redo' in args)
    elif args[0] == 'repaint' and len(args) >= 3:
        repaint(args[1], args[2], '--redo' in args)
    elif args[0] == 'batch' and len(args) >= 3:
        cap = min(float(args[2]), CAPS.get(args[1], float('inf')))
        batch(args[1], cap, [a for a in args[3:] if not a.startswith('--')])
    else:
        print(__doc__)
