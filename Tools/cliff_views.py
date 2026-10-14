"""Writes Saved/TourViews/cliffs.json (Tools/cliff_views.py): a tour view facing each sampled cliff point of an area (layout_computed.json cliffs),
stood out along the point's yaw and looking back at the face; when that spot is outside the playable boundary (some runs'
yaw points into the rock) the view stands on the other side, and a point with neither side inside is skipped. Usage: cliff_views.py
[Area] [out]. For the polish review of cliffs and rocks (2026-10-14)."""
import json, math, os, sys
area = sys.argv[1] if len(sys.argv) > 1 else 'RansomsRest'
out = sys.argv[2] if len(sys.argv) > 2 else None
root = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
out = out or os.path.join(root, 'Saved', 'TourViews', 'cliffs.json')
os.makedirs(os.path.dirname(out), exist_ok=True)
data = json.load(open(os.path.join(root, 'Art', 'Levels', area, 'layout_computed.json'), encoding='utf-8'))
corners = [c[:2] for c in data['boundary']['corners']]


def inside(x, y):
    hit = False
    for i in range(len(corners)):
        (x1, y1), (x2, y2) = corners[i], corners[(i + 1) % len(corners)]
        if (y1 > y) != (y2 > y) and x < (x2 - x1) * (y - y1) / (y2 - y1) + x1:
            hit = not hit
    return hit


views = []
skipped = 0
for run, points in data['cliffs'].items():
    step = 1 if len(points) <= 3 else max(2, round(len(points) / (8 if len(points) > 40 else 5)))
    for i in range(0, len(points), step):
        p = points[i]
        x, y, z = p['location']
        h = p.get('height') or 1200.0
        yaw = p.get('yaw', 0.0)
        d = min(3500.0, max(1300.0, h * 1.4))
        cx, cy = x + math.cos(math.radians(yaw)) * d, y + math.sin(math.radians(yaw)) * d
        if not inside(cx, cy):
            yaw += 180.0
            cx, cy = x + math.cos(math.radians(yaw)) * d, y + math.sin(math.radians(yaw)) * d
            if not inside(cx, cy):
                skipped += 1
                continue
        pitch = max(-12.0, min(30.0, math.degrees(math.atan2(h * 0.45 - 170.0, d))))
        views.append({'name': f'K_{run}_{i:03d}', 'x': round(cx), 'y': round(cy), 'height': 170, 'yaw': round((yaw + 180.0) % 360.0, 1),
                      'pitch': round(pitch, 1), 'fov': 80})
json.dump({'about': 'Cliff review views, written by Tools/cliff_views.py (one per sampled cliff point).', 'views': views},
          open(out, 'w', encoding='utf-8'), indent=1)
print(len(views), 'views ->', out, f'({skipped} points with no side in play skipped)')
