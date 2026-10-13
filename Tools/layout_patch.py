"""Applies the layout patches parallel agents hand in (Art/Levels/<Area>/patches/*.json) to the area's layout.json in
place, keeping the file's own formatting: only what changes is rewritten, in its style. Agents that may not edit
layout.json (another agent owns it) write a patch instead; the orchestrator applies them once the owner is done:

    python Tools/layout_patch.py <Area> [PATCH.json ...] [--check]

With no patch files named, every patches/*.json of the area, in name order. --check prints what would change and writes
nothing. Any Python 3 runs it (Blender's: Tools/terrain_compare.ps1 shows where it lives); the editor isn't needed.

A patch is a JSON list of entries:
- path: dotted keys into layout.json ("gameplay.roosts", "placements");
- op: "set" (replace), "merge" (update a dict's keys), "append" (add to a list);
- value: the new value; comment: why (kept in the patch, not the layout);
- where: {"id": ...} picks the one list element to merge into or set (it must match exactly one);
- unlessKind / unlessId: an append is skipped when the list already holds an element of that kind / id;
- file: "Art/Levels/area_shape.py" marks a table in a Python module (FOOTPRINTS, BUILDINGS): those are listed, not
  applied (edit the module by hand).
An appended element with an id that the list already holds is an error. Apply each patch once: afterwards delete it (the
layout and the git history keep what it did).

After applying: regenerate the terrain if the patch touches the ground (features, roads, placements' footprints,
mesh.holes), then Tools/terrain_check.ps1."""
import json
import os
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
WIDTH = 120
NL = chr(10)


# --- Applying the patches to the layout's data ---

def walk(root, path):
    parts = path.split('.')
    node = root
    for part in parts[:-1]:
        node = node.setdefault(part, {})
    return node, parts[-1]


def apply_patch(data, name, entries, log):
    for entry in entries:
        tag = f"{os.path.basename(name)}: {entry['op']} {entry['path']}"
        if entry.get('file'):
            log(f"by hand ({entry['file']}): {tag}")
            continue
        parent, key = walk(data, entry['path'])
        op, value, where = entry['op'], entry['value'], entry.get('where')
        if where:
            items = parent[key]
            hits = [i for i, x in enumerate(items) if isinstance(x, dict) and all(x.get(k) == w for k, w in where.items())]
            if len(hits) != 1:
                raise ValueError(f'{tag} where {where}: {len(hits)} matches')
            if op == 'merge':
                items[hits[0]].update(value)
            elif op == 'set':
                items[hits[0]] = value
            else:
                raise ValueError(f'{tag}: {op} with where')
            log(f'{tag} {where}')
            continue
        if op == 'set':
            parent[key] = value
        elif op == 'merge':
            parent.setdefault(key, {}).update(value)
        elif op == 'append':
            items = parent.setdefault(key, [])
            if entry.get('unlessKind') and any(isinstance(x, dict) and x.get('kind') == entry['unlessKind'] for x in items):
                log(f"skipped {tag}: a {entry['unlessKind']} is there")
                continue
            if entry.get('unlessId') and any(isinstance(x, dict) and x.get('id') == entry['unlessId'] for x in items):
                log(f"skipped {tag}: {entry['unlessId']} is there")
                continue
            if isinstance(value, dict) and value.get('id') and any(
                    isinstance(x, dict) and x.get('id') == value['id'] for x in items):
                raise ValueError(f"{tag}: the list already has {value['id']}")
            items.append(value)
        else:
            raise ValueError(f'{tag}: unknown op')
        log(tag + (f" {value['id']}" if isinstance(value, dict) and value.get('id') else ''))


# --- A JSON parser that records each value's span ---

class Node:
    def __init__(self, kind, start, end, value=None, items=None, keys=None):
        self.kind, self.start, self.end, self.value = kind, start, end, value
        self.items = items      # list: [Node]
        self.keys = keys        # dict: [(key, key_start, Node)]


def skip(t, i):
    while i < len(t) and t[i] in ' \t\r\n':
        i += 1
    return i


def parse(t, i=0):
    i = skip(t, i)
    c = t[i]
    if c == '{':
        j = skip(t, i + 1)
        keys = []
        if t[j] == '}':
            return Node('dict', i, j + 1, keys=keys)
        while True:
            j = skip(t, j)
            k0 = j
            key, j = json.decoder.scanstring(t, j + 1)
            j = skip(t, j)
            assert t[j] == ':'
            node = parse(t, j + 1)
            keys.append((key, k0, node))
            j = skip(t, node.end)
            if t[j] == ',':
                j += 1
                continue
            assert t[j] == '}', t[j - 40:j + 40]
            return Node('dict', i, j + 1, keys=keys)
    if c == '[':
        j = skip(t, i + 1)
        items = []
        if t[j] == ']':
            return Node('list', i, j + 1, items=items)
        while True:
            node = parse(t, j)
            items.append(node)
            j = skip(t, node.end)
            if t[j] == ',':
                j += 1
                continue
            assert t[j] == ']'
            return Node('list', i, j + 1, items=items)
    value, end = json.JSONDecoder().raw_decode(t, i)
    return Node('scalar', i, end, value=value)


# --- Formatting new values in the file's style ---

def num(x):
    if isinstance(x, bool):
        return 'true' if x else 'false'
    if isinstance(x, int):
        return str(x)
    if float(x).is_integer() and abs(x) < 1e15:
        return str(int(x)) if not isinstance(x, float) else repr(x)
    return repr(x)


def compact(v):
    if isinstance(v, dict):
        return '{ ' + ', '.join(f'{json.dumps(k)}: {compact(x)}' for k, x in v.items()) + ' }' if v else '{}'
    if isinstance(v, list):
        return '[' + ','.join(compact(x) for x in v) + ']'
    if isinstance(v, (int, float)) and not isinstance(v, bool):
        return num(v)
    return json.dumps(v, ensure_ascii=False)


def fmt(v, ind, col=None):
    """v formatted: on one line when it fits from column col (ind by default), else over lines indented from ind."""
    one = compact(v)
    if (ind if col is None else col) + len(one) <= WIDTH:
        return one
    pad = ' ' * (ind + 2)
    if isinstance(v, dict):
        lines = [f'{pad}{json.dumps(k)}: {fmt(x, ind + 2, ind + 2 + len(json.dumps(k)) + 2)}' for k, x in v.items()]
        return '{\n' + ',\n'.join(lines) + '\n' + ' ' * ind + '}'
    if isinstance(v, list):
        if all(isinstance(x, list) for x in v):
            # Point lists, wrapped at the width.
            rows, cur = [], ''
            for x in v:
                piece = compact(x)
                if cur and len(pad) + len(cur) + len(piece) + 1 > WIDTH:
                    rows.append(cur)
                    cur = ''
                cur += piece + ','
            rows.append(cur.rstrip(','))
            return '[\n' + '\n'.join(pad + r for r in rows) + '\n' + ' ' * ind + ']'
        return '[\n' + ',\n'.join(pad + fmt(x, ind + 2) for x in v) + '\n' + ' ' * ind + ']'
    return one


def indent_at(t, pos):
    line = t.rfind('\n', 0, pos) + 1
    k = line
    while t[k] == ' ':
        k += 1
    return k - line



# --- Writing the patched data back in the file's style ---

def diff(t, node, target, edits):
    """Edits (start, end, text) that make node's text equal target."""
    inline = NL not in t[node.start:node.end]
    if node.kind == 'dict' and isinstance(target, dict) and not (inline and set(target) - {k for k, _, _ in node.keys}):
        have = {k for k, _, _ in node.keys}
        names = [k for k, _, _ in node.keys]
        for idx, (k, k0, child) in enumerate(node.keys):
            if k not in target:
                # Delete the key with the comma before it (or after it, for the first).
                if idx > 0:
                    edits.append((node.keys[idx - 1][2].end, child.end, ''))
                else:
                    nxt = node.keys[1][1] if len(node.keys) > 1 else child.end
                    edits.append((k0, nxt, ''))
            else:
                diff(t, child, target[k], edits)
        new = [k for k in target if k not in have]
        if new:
            last = node.keys[-1][2].end if node.keys else node.start + 1
            ind = indent_at(t, node.keys[-1][1]) if node.keys else indent_at(t, node.start) + 2
            text = ''.join(',' + NL + ' ' * ind + json.dumps(k) + ': ' + fmt(target[k], ind, ind + len(json.dumps(k)) + 2)
                           for k in new)
            edits.append((last, last, text))
        del names
        return
    if node.kind == 'list' and isinstance(target, list):
        values = [json.loads(t[c.start:c.end]) for c in node.items]
        n = len(values)
        same_prefix = all(_same(values[i], target[i]) for i in range(n)) if len(target) >= n else False
        dict_prefix = (len(target) >= n and n and all(isinstance(values[i], dict) and isinstance(target[i], dict)
                                                       for i in range(n)))
        if n and len(target) >= n and (same_prefix or dict_prefix):
            # The items it had (each patched in place when they're objects), then the new ones after them.
            if not same_prefix:
                for c, x in zip(node.items, target[:n]):
                    diff(t, c, x, edits)
            if len(target) > n:
                ind = indent_at(t, node.items[-1].start)
                one_line = t.rfind(NL, node.start, node.items[-1].start) < node.start
                if one_line:
                    text = ''.join(',' + compact(x) for x in target[n:])
                else:
                    text = ''.join(',' + NL + ' ' * ind + fmt(x, ind) for x in target[n:])
                edits.append((node.items[-1].end, node.items[-1].end, text))
            return
    current = json.loads(t[node.start:node.end])
    if _same(current, target):
        return
    column = node.start - (t.rfind(NL, 0, node.start) + 1)
    edits.append((node.start, node.end, fmt(target, indent_at(t, node.start), column)))




def _same(a, b):
    return json.dumps(a, sort_keys=True) == json.dumps(b, sort_keys=True)


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    check = '--check' in sys.argv
    if not args:
        print(__doc__)
        sys.exit(1)
    folder = os.path.join(ROOT, 'Art', 'Levels', args[0])
    patches = args[1:] or sorted(os.path.join(folder, 'patches', f) for f in os.listdir(os.path.join(folder, 'patches'))
                                 if f.endswith('.json'))
    layout_path = os.path.join(folder, 'layout.json')
    original = open(layout_path, encoding='utf-8', newline='').read()  # its own line endings kept
    target = json.loads(original)
    for name in patches:
        apply_patch(target, name, json.load(open(name, encoding='utf-8')), print)
    root = parse(original)
    edits = []
    diff(original, root, target, edits)
    out = original
    newline = chr(13) + chr(10) if chr(13) + chr(10) in original else chr(10)
    for start, end, text in sorted(edits, key=lambda e: (e[0], e[1]), reverse=True):
        out = out[:start] + text.replace(chr(10), newline) + out[end:]
    if not _same(json.loads(out), target):
        raise SystemExit('the patched file does not equal the patched data: nothing written')
    if check:
        print(f'{len(edits)} edits (--check: nothing written)')
        return
    open(layout_path, 'w', encoding='utf-8', newline='').write(out)
    print(f'{len(edits)} edits written to {layout_path}')


if __name__ == '__main__':
    main()
