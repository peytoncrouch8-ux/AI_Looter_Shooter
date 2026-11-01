"""Tripo 3D (https://developers.tripo3d.com, API v3) for the models the user approves (2026-10-31: "don't use credits from
tripo unless approved"). The key is the user's TRIPO_API_KEY (their Windows user environment variable, read from the
registry when this process started before it was set); it is never printed or written anywhere.

Usage (UE's bundled python works: no packages needed):
  python Tools/tripo.py balance
  python Tools/tripo.py make <Name> [--no-rig] [--paint]   generate the model in SPECS (and rig a person onto Unreal's
                                                  skeleton); --paint repaints it from its spec's paint words first
                                                  (and, with the spec's paint_style image, in that picture's style)
  python Tools/tripo.py status <task_id>

Every step prints the credits it used and the balance after. Results land in Art/Tripo/<Name>/: task.json (every task id,
its status and credits), <Name>.glb (the textured model), <Name>_rigged.fbx (people: rigged with spec "unreal", for the
SK_Mannequin skeleton), preview.png (Tripo's render). Re-running `make` reuses finished tasks recorded in task.json, so a
crash never pays twice.
"""

import json
import os
import sys
import time
import urllib.error
import urllib.request

BASE = 'https://openapi.tripo3d.ai/v3'  # the global API (.com is China's; the key is the global one)
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, 'Art', 'Tripo')

# The game's look: textured "stylized realism" (CLAUDE.md), the Weird West frontier of the 1880s.
STYLE = 'stylized realistic game asset, hand-painted look with realistic materials, American frontier 1880s'
PERSON = ('full body single character, standing in A-pose, arms held out about 45 degrees from the body, feet apart, '
          'empty open hands, facing forward, neutral expression')
PERSON_NO = 'weapon, gun, holding objects, props in hands, T-pose, base, platform, pedestal, multiple characters, cropped'
# Delia's first texture came out grey like clay: every spec now names its colours ("in full colour") up front.
COLOUR = 'Painted in full natural colour, realistic materials.'
PROP_NO = 'person, character, hands, base, platform, pedestal, ground plane, text, watermark, multiple objects'

# What the user approved to make; prompts follow Docs/Work/2026-10-31-Wardens/TRIPO_MODELS.md. Polycounts for the
# game's 120 fps budget (Medium draws the Nanite fallback): people ~40k triangles.
SPECS = {
    'Delia': dict(
        prompt=f'{PERSON}. An elderly frontier woman in her seventies, small and tough, weathered kind face, grey hair '
               f'in a tight bun, a dark wool shawl over a long faded work dress, a work apron, worn leather boots. {STYLE}',
        negative=PERSON_NO, faces=40000, person=True,
        # Her first texture came out grey like clay (2026-10-31); the repaint names every colour.
        paint='Painted in full natural colour: weathered tanned skin with rosy cheeks, silver-grey hair in a bun, a dark '
              'chocolate-brown wool shawl, a faded dusty-blue cotton work dress, a cream linen apron with old stains, '
              'worn brown leather boots, a dark leather belt. Realistic fabric and skin, warm frontier sunlight colours.'),

    # --- People (rigged with spec "unreal" for the SK_Mannequin skeleton) ---
    'Holloway': dict(
        prompt=f'{PERSON}. Captain Mercy Holloway, an airship captain, a tall strong woman in her forties, a pale scar '
               f'across her left cheek, dark brown hair tied back, a long weathered dark navy-blue captain\'s coat with '
               f'brass buttons, a peaked captain\'s hat with a small brass lantern badge, a cream shirt, a brown leather '
               f'belt, dark grey trousers, tall black boots, brown leather gloves. {COLOUR} {STYLE}',
        negative=PERSON_NO, faces=40000, person=True),
    'Finch': dict(
        prompt=f'{PERSON}. Brother Tobiah Finch, a thin older lantern-keeper monk with a kind lined face, short grey '
               f'beard, round brass spectacles, a plain undyed oatmeal-brown hooded wool robe worn over dark trousers, '
               f'worn brown boots, a leather belt hung with small brass tools, wicks and a tiny oil can. {COLOUR} {STYLE}',
        negative=PERSON_NO, faces=40000, person=True),
    'Junie': dict(
        prompt=f'{PERSON}. Junie Calloway, a young airship engineer woman, freckles, short red-brown hair, brass '
               f'goggles pushed up on her forehead, a red kerchief round her neck, faded olive-green canvas work overalls '
               f'with rolled sleeves over a white shirt, oil stains, heavy brown work gloves, scuffed brown boots. '
               f'{COLOUR} {STYLE}',
        negative=PERSON_NO, faces=40000, person=True),
    'DredgerRifleman': dict(
        prompt=f'{PERSON}. A sinister industrial company soldier, a black glossy rubber raincoat to the knees, a black '
               f'gas mask with two round glass lenses glowing faint green and a short filter snout, a flat dark grey '
               f'company cap, a small brass badge on the chest, a brown leather belt with ammo pouches, heavy black '
               f'boots, black rubber gloves. {COLOUR} {STYLE}',
        negative=PERSON_NO, faces=35000, person=True),
    'DredgerHoseMan': dict(
        prompt=f'{PERSON}. A bulky sinister industrial company worker, a black glossy rubber coat under a heavy brown '
               f'leather apron, a black gas mask with two round glass lenses and a filter snout, a big brass and iron '
               f'tank on his back with gauges, a valve and pipes, black rubber gloves, heavy black boots. {COLOUR} {STYLE}',
        negative=PERSON_NO, faces=40000, person=True),

    # --- Props held or used ---
    'DeliaShotgun': dict(
        prompt=f'An old double-barrel shotgun, side view, worn walnut wood stock, dull dark grey steel barrels, brass '
               f'trigger guard, 1880s frontier. {COLOUR} {STYLE}', negative=PROP_NO, faces=12000),
    'CompanyRifle': dict(
        prompt=f'A lever-action company rifle, side view, blued dark steel, dark stained wood stock with a small brass '
               f'plate, a short brass scope on top, industrial and sinister, 1880s. {COLOUR} {STYLE}',
        negative=PROP_NO, faces=12000),
    'HoseNozzle': dict(
        prompt=f'A brass industrial suction nozzle with a pistol grip and trigger, attached to a short ribbed black '
               f'rubber hose, steampunk machinery, polished and dented brass. {COLOUR} {STYLE}',
        negative=PROP_NO, faces=10000),
    'GreatLantern': dict(
        prompt=f'A big ornate brass ship lantern on a swivel mount, glass panes in a brass cage, a domed cap with a '
               f'handle ring, warm golden light inside, aged polished brass, 1.5 metres tall. {COLOUR} {STYLE}',
        negative=PROP_NO, faces=20000),

    # --- Ships ---
    'Lamplighter': dict(
        prompt=f'A small adventurers\' airship, side three-quarter view: a long cream canvas gas-bag in rope netting '
               f'above a wooden boat hull painted dark teal with brass trim, a flat open walkable wooden deck with rails, '
               f'a small wooden wheelhouse at the back with a ship\'s wheel, an engine housing with two propellers at '
               f'the stern, a big brass lantern at the prow, lived in and weathered. {COLOUR} {STYLE}',
        negative=PROP_NO + ', water, sea', faces=80000),
    'DredgeBarge': dict(
        prompt=f'A huge ugly industrial airship barge: a riveted rusty iron hull with brown and black rust streaks, '
               f'two tall black smokestacks, furnace vents glowing orange, cranes, a big hose drum underneath, two '
               f'grey patched gas-bags above, faded red company paint, menacing and dirty. {COLOUR} {STYLE}',
        negative=PROP_NO + ', water, sea', faces=80000),
    'DredgeHose': dict(
        prompt=f'One straight section of a giant industrial hose, a black ribbed rubber tube with thick rusty iron '
               f'collars at both ends, about ten times as long as it is wide. {COLOUR} {STYLE}',
        negative=PROP_NO, faces=8000),
    'HoseIntake': dict(
        prompt=f'The iron intake mouth at the end of a giant industrial suction hose: a flared rusty iron funnel with '
               f'a grate across it and rivets, oily black stains. {COLOUR} {STYLE}', negative=PROP_NO, faces=10000),
    'CompanyTender': dict(
        prompt=f'A small boarding airship skiff: a narrow riveted dark iron hull with rust, a small grey gas-bag above '
               f'in netting, two harpoon grapple launchers on the bow, a small engine with a propeller, industrial and '
               f'menacing. {COLOUR} {STYLE}', negative=PROP_NO + ', water, sea', faces=40000),
    'GrappleHook': dict(
        prompt=f'A big black iron three-pronged grappling hook with a heavy chain eye at the top, rusty and '
               f'battered. {COLOUR} {STYLE}', negative=PROP_NO, faces=8000),

    # --- World pieces ---
    'AnchorWinch': dict(
        prompt=f'A ground mooring winch: a thick weathered wooden post with a rusty iron winch drum wound with rope, '
               f'a crank handle and iron brackets bolted to a wooden base. {COLOUR} {STYLE}',
        negative=PROP_NO, faces=12000),
    'BarricadeTable': dict(
        prompt=f'An overturned wooden farmhouse kitchen table lying on its side as a barricade, worn pine wood with old '
               f'scratches, bullet holes. {COLOUR} {STYLE}', negative=PROP_NO, faces=8000),
    'BarricadeBoards': dict(
        prompt=f'A barricade of rough weathered wooden planks nailed across two posts, uneven grey-brown boards, rusty '
               f'nails. {COLOUR} {STYLE}', negative=PROP_NO, faces=8000),
    'BarricadeSacks': dict(
        prompt=f'A pile of five stacked burlap flour sacks, tan coarse cloth tied with string, one slumped and '
               f'dusty. {COLOUR} {STYLE}', negative=PROP_NO, faces=8000,
        # Came out grey like clay (2026-11-01): repainted.
        paint='Painted in full natural colour: tan and oatmeal coarse burlap sacks with brown stitching and twine ties, '
              'white flour dust on the folds, one sack with a faded red mill stamp, warm frontier light.'),
    'LoudspeakerPost': dict(
        prompt=f'A tall weathered wooden pole with two big brass horn loudspeakers at the top pointing in different '
               f'directions, wires running down the pole, a small brass plate, industrial, 4 metres tall. {COLOUR} '
               f'{STYLE}', negative=PROP_NO, faces=12000),
    'AbelsHat': dict(
        prompt=f'A weathered wide-brim frontier hat, worn dark brown felt, a sooty band, dented crown, dusty. '
               f'{COLOUR} {STYLE}', negative=PROP_NO, faces=6000),
    'LanternPost': dict(
        prompt=f'A tall weathered wooden lantern post, 3 metres, on a cross-shaped wooden foot, a braced wooden arm '
               f'reaching out with an iron lantern hanging from its end (glass panes, iron cage, a cap), a small iron '
               f'bell hook under the arm, a lantern badge burnt into the post. {COLOUR} {STYLE}',
        negative=PROP_NO, faces=12000),
}

# The batch order (people first; then what Landfall shows most).
ORDER = ['Holloway', 'Finch', 'Junie', 'DredgerRifleman', 'DredgerHoseMan', 'GreatLantern', 'Lamplighter',
         'DredgeBarge', 'CompanyTender', 'AnchorWinch', 'GrappleHook', 'DredgeHose', 'HoseIntake', 'DeliaShotgun',
         'CompanyRifle', 'HoseNozzle', 'LanternPost', 'LoudspeakerPost', 'AbelsHat', 'BarricadeTable',
         'BarricadeBoards', 'BarricadeSacks']


def key():
    value = os.environ.get('TRIPO_API_KEY')
    if not value and sys.platform == 'win32':
        import winreg
        try:
            with winreg.OpenKey(winreg.HKEY_CURRENT_USER, 'Environment') as env:
                value = winreg.QueryValueEx(env, 'TRIPO_API_KEY')[0]
        except OSError:
            value = None
    if not value:
        sys.exit('No TRIPO_API_KEY: set it in your Windows user environment variables.')
    return value.strip()


def call(method, path, body=None):
    data = json.dumps(body).encode() if body is not None else None
    request = urllib.request.Request(BASE + path, data=data, method=method, headers={
        'Authorization': 'Bearer ' + key(), 'Content-Type': 'application/json'})
    try:
        with urllib.request.urlopen(request, timeout=60) as response:
            reply = json.loads(response.read().decode())
    except urllib.error.HTTPError as error:
        sys.exit(f'Tripo {method} {path}: HTTP {error.code}: {error.read().decode(errors="replace")[:400]}')
    if reply.get('code') != 0:
        sys.exit(f'Tripo {method} {path}: {json.dumps(reply)[:400]}')
    return reply.get('data', {})


def upload(path):
    """Uploads an image (PNG/JPEG, at least 256 px) and returns its file_token (POST /files, multipart field 'file')."""
    boundary = '----looter' + str(int(time.time() * 1000))
    with open(path, 'rb') as f:
        data = f.read()
    kind = 'image/png' if path.lower().endswith('.png') else 'image/jpeg'
    body = (f'--{boundary}\r\nContent-Disposition: form-data; name="file"; filename="{os.path.basename(path)}"\r\n'
            f'Content-Type: {kind}\r\n\r\n').encode() + data + f'\r\n--{boundary}--\r\n'.encode()
    request = urllib.request.Request(BASE + '/files', data=body, method='POST', headers={
        'Authorization': 'Bearer ' + key(), 'Content-Type': f'multipart/form-data; boundary={boundary}'})
    try:
        with urllib.request.urlopen(request, timeout=120) as response:
            reply = json.loads(response.read().decode())
    except urllib.error.HTTPError as error:
        sys.exit(f'Tripo upload {path}: HTTP {error.code}: {error.read().decode(errors="replace")[:400]}')
    if reply.get('code') != 0:
        sys.exit(f'Tripo upload {path}: {json.dumps(reply)[:400]}')
    token = reply['data']['file_token']
    print(f'  uploaded {os.path.basename(path)}', flush=True)
    return token


def balance():
    data = call('GET', '/account/balance')
    return data.get('balance'), data.get('frozen')


def wait(task_id, what):
    print(f'  {what}: task {task_id}', flush=True)
    last = -1
    while True:
        time.sleep(3)
        data = call('GET', f'/tasks/{task_id}')
        status, progress = data.get('status'), data.get('progress', 0)
        if progress != last:
            print(f'    {status} {progress}%', flush=True)
            last = progress
        if status == 'success':
            print(f'  {what} done: {data.get("credits_consumed")} credits', flush=True)
            return data
        if status in ('failed', 'cancelled', 'banned'):
            sys.exit(f'  {what} {status}: {json.dumps(data)[:400]}')


def download(url, path):
    with urllib.request.urlopen(url, timeout=300) as response, open(path, 'wb') as out:
        out.write(response.read())
    print(f'  saved {os.path.relpath(path, ROOT)} ({os.path.getsize(path) // 1024} KB)', flush=True)


def make(name, rig=True, paint=False):
    """Makes the model (and its repaint and rig as asked); returns the credits its own tasks used."""
    used = 0.0
    spec = SPECS.get(name)
    if spec is None:
        sys.exit(f'No spec for {name}: known {sorted(SPECS)}')
    folder = os.path.join(OUT, name)
    os.makedirs(folder, exist_ok=True)
    record_path = os.path.join(folder, 'task.json')
    record = json.load(open(record_path)) if os.path.exists(record_path) else {}

    def save():
        json.dump(record, open(record_path, 'w'), indent=2)

    before = balance()
    print(f'{name}: balance {before[0]} (frozen {before[1]})', flush=True)

    if 'model' not in record or record['model'].get('status') != 'success':
        body = dict(prompt=spec['prompt'], model='v3.1-20260211', texture=True, pbr=True, texture_quality='detailed',
                    geometry_quality='standard', face_limit=spec['faces'], auto_size=True)
        if spec.get('negative'):
            body['negative_prompt'] = spec['negative']
        task_id = call('POST', '/generation/text-to-model', body)['task_id']
        record['model'] = dict(task_id=task_id, status='running')
        save()
        result = wait(task_id, 'model')
        record['model'] = dict(task_id=task_id, status='success', credits=result.get('credits_consumed'))
        used += float(result.get('credits_consumed') or 0)
        save()
        output = result.get('output', {})
        download(output['model_url'], os.path.join(folder, f'{name}.glb'))
        if output.get('rendered_image_url'):
            download(output['rendered_image_url'], os.path.join(folder, 'preview.png'))

    if paint and spec.get('paint') and record.get('paint', {}).get('status') != 'success':
        body = dict(input=record['model']['task_id'], model='v3.0-20250812', texture_prompt=dict(text=spec['paint']),
                    pbr=True, texture_quality='detailed', texture_alignment='geometry')
        if spec.get('paint_style'):
            # A picture of the game's own look (a crop of a building in game, a preview render): Tripo paints in its
            # style (texture_prompt.style_image, read with text only).
            body['texture_prompt']['style_image'] = {'file_token': upload(os.path.join(ROOT, spec['paint_style']))}
        task_id = call('POST', '/models/texture', body)['task_id']
        record['paint'] = dict(task_id=task_id, status='running')
        record.pop('rig', None)
        save()
        result = wait(task_id, 'repaint')
        record['paint'] = dict(task_id=task_id, status='success', credits=result.get('credits_consumed'))
        used += float(result.get('credits_consumed') or 0)
        save()
        glb = os.path.join(folder, f'{name}.glb')
        if os.path.exists(glb):
            os.replace(glb, os.path.join(folder, f'{name}_grey.glb'))
        output = result.get('output', {})
        download(output['model_url'], glb)
        if output.get('rendered_image_url'):
            download(output['rendered_image_url'], os.path.join(folder, 'preview_painted.png'))

    if rig and spec.get('person') and record.get('rig', {}).get('status') != 'success':
        # Rig the painted model when there is one.
        model_task = record.get('paint', {}).get('task_id') or record['model']['task_id']
        check = call('POST', '/animations/rig-check', {'input': model_task})
        check_id = check.get('task_id')
        verdict = wait(check_id, 'rig check').get('output', {}) if check_id else check
        print(f'  rig check: {verdict}', flush=True)
        if verdict.get('riggable') is False:
            sys.exit('  Tripo says it is not riggable: no rig made.')
        body = dict(input=model_task, model='v3.0-20260909', rig_type=verdict.get('rig_type') or 'biped', spec='unreal',
                    out_format='fbx')
        task_id = call('POST', '/animations/rig', body)['task_id']
        record['rig'] = dict(task_id=task_id, status='running')
        save()
        result = wait(task_id, 'rig')
        record['rig'] = dict(task_id=task_id, status='success', credits=result.get('credits_consumed'))
        used += float(result.get('credits_consumed') or 0)
        save()
        download(result['output']['model_url'], os.path.join(folder, f'{name}_rigged.fbx'))

    after = balance()
    print(f'{name}: its own tasks used {used} credits', flush=True)
    print(f'{name}: done. Balance {after[0]} (was {before[0]}: {before[0] - after[0] if None not in (before[0], after[0]) else "?"} '
          f'credits used)', flush=True)
    return used


SPEC_DIR = os.path.join(OUT, 'specs')


def load_spec_files():
    """Adds every Art/Tripo/specs/<job>.json ({name: spec}) to SPECS; returns {job: [names in file order]}."""
    jobs = {}
    if os.path.isdir(SPEC_DIR):
        for file in sorted(os.listdir(SPEC_DIR)):
            if file.endswith('.json'):
                with open(os.path.join(SPEC_DIR, file), encoding='utf-8') as handle:
                    specs = json.load(handle)
                SPECS.update(specs)
                jobs[file[:-5]] = list(specs)
    return jobs


def batch(cap, names=None):
    spent = 0.0
    for name in names or ORDER:
        record_path = os.path.join(OUT, name, 'task.json')
        if os.path.exists(record_path):
            record = json.load(open(record_path))
            done = record.get('model', {}).get('status') == 'success' and (
                not SPECS[name].get('person') or record.get('rig', {}).get('status') == 'success')
            if done:
                continue
        if spent + (60 if SPECS[name].get('person') else 40) > cap:
            print(f'BATCH stop before {name}: {spent} credits spent, cap {cap}', flush=True)
            return
        try:
            spent += make(name)
        except SystemExit as stop:
            print(f'BATCH {name} failed: {stop}', flush=True)
    print(f'BATCH done: {spent} credits spent by this batch', flush=True)


if __name__ == '__main__':
    args = sys.argv[1:]
    if not args or args[0] == 'balance':
        value, frozen = balance()
        print(f'Balance {value}, frozen {frozen}')
    elif args[0] == 'make' and len(args) >= 2:
        load_spec_files()
        make(args[1], rig='--no-rig' not in args, paint='--paint' in args)
    elif args[0] == 'batch':
        # batch <cap> [job]: a job's spec file's models (Art/Tripo/specs/<job>.json), else the Wardens list.
        spec_jobs = load_spec_files()
        job = args[2] if len(args) > 2 else None
        if job and job not in spec_jobs:
            sys.exit(f'No Art/Tripo/specs/{job}.json (known {sorted(spec_jobs)})')
        batch(float(args[1]) if len(args) > 1 else 1300.0, spec_jobs.get(job) if job else None)
    elif args[0] == 'status' and len(args) == 2:
        print(json.dumps(call('GET', f'/tasks/{args[1]}'), indent=2))
    else:
        print(__doc__)
