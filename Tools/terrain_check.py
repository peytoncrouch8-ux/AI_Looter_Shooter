"""Checks an area's layout and its computed layout before anything is built from them. Run it with Blender's own Python,
which has numpy (Tools/terrain_check.ps1 finds it); the editor isn't needed:

    Tools/terrain_check.ps1 <Area> [-Computed PATH]
    <Blender>/<version>/python/bin/python.exe Tools/terrain_check.py <Area> [--layout PATH] [--computed PATH]

From Art/Levels/<Area>/layout.json (the generator's own geometry, nothing built):
- every feature of a known type with what it needs: a mesa has no ramp, a pit has one (or says "noRamp"), a ridge's
  heights match its path, a scarp's step is 2-6 m, a falls' gorge starts at its creek's end; ids and cliff groups
  unique; every ramp at least RAMP_MIN_WIDTH wide;
- grounded: features, roads, ramps, footprints and the playable boundary clear of the seam band (the core's outer
  region.seamBand, where it fades into the regional field), the lip crossing the core square once, and the boundary a
  simple polygon whose open runs name real corners.
From Art/Levels/<Area>/layout_computed.json (written by Art/Models/Terrain/<Area>.py --computed):
- that it was computed from this layout.json (its layoutSha1: area_shape.layout_sha1, which leaves out the blocks the
  generator never reads, "level" and "gameplay");
- every ramp's grade (over RAMP_WINDOW m) at most RAMP_MAX_GRADE (warned past RAMP_WARN_GRADE) and its width;
- every cliff course at most MAX_COURSE (the cliff kit's tallest piece), every wall taller than that dressed in
  courses that add up to it, every feature that has cliffs given a dressed group;
- each falls dropping, the boundary's corners and open edges matching, and the open-ground metric against its target
  (a warning: it's the design's aim, not a rule).
It prints each finding and a verdict, and exits with 1 on any error.
"""
import argparse
import json
import math
import os
import sys

import numpy as np

REPO = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
sys.path.insert(0, os.path.join(REPO, 'Art', 'Levels'))

import area_features  # noqa: E402
import area_region  # noqa: E402
import area_shape  # noqa: E402

MAX_COURSE = 1200.0      # cm: the cliff kit's tallest piece (CliffFace_D)
MAX_OUTCROP = 1250.0     # cm: the outcrop kit's tallest piece (the 12.4 m spine)
RAMP_MIN_WIDTH = 300.0   # cm
RAMP_WARN_WIDTH = 400.0
RAMP_MAX_GRADE = 0.35    # rise over run: about 19 degrees, well under the walkable angle, at an easy walk
RAMP_WARN_GRADE = 0.30
RAMP_WINDOW = 300.0      # cm: grades are measured over at least this run
SCARP_RANGE = (200.0, 600.0)
FALLS_JOIN = 200.0       # cm: a gorge starts this close to its creek's end


class Findings:
    def __init__(self):
        self.errors, self.warnings, self.notes = [], [], []

    def error(self, text):
        self.errors.append(text)
        print(f'  ERROR    {text}')

    def warn(self, text):
        self.warnings.append(text)
        print(f'  warning  {text}')

    def ok(self, text):
        self.notes.append(text)
        print(f'  ok       {text}')


def check_layout(area, out):
    layout = area.layout
    print(f'{area.path}')
    ids = [f['id'] for f in layout['features']]
    for dup in sorted({i for i in ids if ids.count(i) > 1}):
        out.error(f'two features are called {dup!r}')
    groups = []
    for f in layout['features']:
        kind, fid = f['type'], f['id']
        if kind in ('plateau', 'mesa', 'pit', 'ridge', 'scarp', 'knob', 'gully'):
            groups.append(f.get('cliffGroup', fid))
        ramp = f.get('ramp')
        if ramp:
            groups.append(ramp.get('cliffGroup', ramp.get('id', fid + '_ramp')))
            if ramp['width'] < RAMP_MIN_WIDTH:
                out.error(f'{kind} {fid}: its ramp is {ramp["width"] / 100.0:g} m wide (at least '
                          f'{RAMP_MIN_WIDTH / 100.0:g} m)')
            elif ramp['width'] < RAMP_WARN_WIDTH:
                out.warn(f'{kind} {fid}: its ramp is {ramp["width"] / 100.0:g} m wide; '
                         f'{RAMP_WARN_WIDTH / 100.0:g} m lets two pass')
        if kind in ('plateau', 'mesa', 'pit') and len(f.get('polygon', [])) < 3:
            out.error(f'{kind} {fid}: needs a polygon of at least three points')
        if kind == 'mesa' and ramp:
            out.error(f'mesa {fid}: a mesa has no ramp (make it a plateau)')
        if kind == 'pit' and not ramp and not f.get('noRamp'):
            out.error(f'pit {fid}: has no ramp down (give it one, or "noRamp": true if nothing should go in)')
        if kind == 'pit' and 'depth' not in f and 'floor' not in f:
            out.error(f'pit {fid}: needs a depth (or a floor height)')
        if kind == 'ridge':
            if 'heights' in f and len(f['heights']) != len(f['path']):
                out.error(f'ridge {fid}: {len(f["heights"])} heights for {len(f["path"])} path points')
            if f.get('cliffs', 'both') not in ('left', 'right', 'both', 'none'):
                out.error(f'ridge {fid}: cliffs must be left, right, both or none')
        if kind == 'scarp':
            if not SCARP_RANGE[0] <= f['height'] <= SCARP_RANGE[1]:
                out.warn(f'scarp {fid}: a {f["height"] / 100.0:g} m step (a scarp is 2-6 m; taller is a plateau)')
            if f.get('side', 'left') not in ('left', 'right'):
                out.error(f'scarp {fid}: side must be left or right (the high side, looking along the path)')
        if kind == 'knob' and f.get('rock', {}).get('height', 0) > MAX_OUTCROP:
            out.error(f'knob {fid}: its rock is {f["rock"]["height"] / 100.0:g} m (the outcrop kit stops at '
                      f'{MAX_OUTCROP / 100.0:g} m)')
        if kind == 'gully' and f.get('depth', 0) <= 0:
            out.error(f'gully {fid}: needs a depth')
        falls = f.get('falls') if kind == 'creek' else None
        if falls:
            gorge = falls.get('gorge', {})
            groups.append(gorge.get('cliffGroup', gorge.get('id', fid + 'Gorge')))
            if falls.get('drop', 0) <= 0 or 'path' not in gorge:
                out.error(f'creek {fid}: a falls needs a drop and a gorge path')
            elif math.dist(f['path'][-1], gorge['path'][0]) > FALLS_JOIN:
                out.error(f'creek {fid}: its gorge starts {math.dist(f["path"][-1], gorge["path"][0]) / 100.0:.1f} m '
                          f'from the creek\'s end (its lip); start it there')
    for dup in sorted({g for g in groups if groups.count(g) > 1}):
        out.error(f'two features share the cliff group {dup!r}: give one a cliffGroup')
    if area.setting == 'grounded':
        check_grounded(area, out)
    if not out.errors:
        out.ok(f'{len(layout["features"])} features of known types, each with what it needs')


def check_grounded(area, out):
    region = area.region
    conflicts = area_features.seam_conflicts(area)
    for text in conflicts:
        out.error(text)
    if not conflicts:
        out.ok(f'every feature, road, ramp, footprint and the boundary clear of the {region.seam:g} m seam band '
               f'(within {area.half - region.seam:g} m of the center on the upland)')
    try:
        _, on_lip = region.core_outline()
        out.ok('the escarpment\'s lip crosses the core square once' if on_lip.any() else
               'no escarpment crosses the core square')
    except ValueError as error:
        out.error(f'the core\'s edge: {error}')
    try:
        corners, flags = area_region.boundary(area)
    except ValueError as error:
        out.error(str(error))
        return
    if corners is None:
        out.warn('no playable boundary (layout.json "boundary"): the game will place no APlayableArea')
        return
    crossing = area_region.edges_cross(corners)
    for i, j in crossing:
        out.error(f'the boundary\'s edges {i} and {j} cross')
    if not crossing:
        out.ok(f'the boundary: {len(corners)} corners, a simple polygon, {sum(flags)} open edges')


def grades(points):
    """The steepest grade along a polyline of [x, y, z] (cm) over runs of at least RAMP_WINDOW, its length (cm) and
    its average grade."""
    p = np.asarray(points, dtype=np.float64)
    s = np.concatenate([[0.0], np.cumsum(np.linalg.norm(np.diff(p[:, :2], axis=0), axis=1))])
    steepest = 0.0
    for i in range(len(p)):
        j = int(np.searchsorted(s, s[i] + RAMP_WINDOW))
        if j >= len(p):
            break
        steepest = max(steepest, abs(p[j, 2] - p[i, 2]) / (s[j] - s[i]))
    return steepest, s[-1], abs(p[-1, 2] - p[0, 2]) / max(s[-1], 1e-6)


def check_computed(area, computed, out):
    print(f'{computed}')
    if not os.path.exists(computed):
        out.error(f'no {os.path.basename(computed)}: run Art/Models/Terrain/{area.name}.py with --computed')
        return
    with open(computed, encoding='utf-8') as file:
        data = json.load(file)
    sha = area_shape.layout_sha1(area.path)
    if 'layoutSha1' not in data:
        out.warn('it doesn\'t say which layout.json it came from (no layoutSha1): rebuild it to be sure')
    elif data['layoutSha1'] != sha:
        out.error('it was computed from another layout.json: run the terrain model with --computed again')
    else:
        out.ok('computed from this layout.json')
    # Ramps.
    ramps = data.get('ramps') or ({'ramp': data['ramp']} if data.get('ramp') else {})
    for rid, ramp in ramps.items():
        steepest, length, average = grades(ramp['points'])
        text = (f'ramp {rid}: {length / 100.0:.1f} m long, {ramp["width"] / 100.0:g} m wide, {average * 100.0:.0f}% '
                f'on average, {steepest * 100.0:.0f}% at its steepest')
        if ramp['width'] < RAMP_MIN_WIDTH or steepest > RAMP_MAX_GRADE:
            out.error(text + f' (at most {RAMP_MAX_GRADE * 100.0:.0f}%, at least {RAMP_MIN_WIDTH / 100.0:g} m wide)')
        elif steepest > RAMP_WARN_GRADE:
            out.warn(text + f' (steeper than {RAMP_WARN_GRADE * 100.0:.0f}%)')
        else:
            out.ok(text)
    # Cliff courses.
    counted = tallest = 0
    for group, points in data.get('cliffs', {}).items():
        for i, point in enumerate(points):
            kind = point.get('kind')
            if kind == 'bank':
                continue
            if kind == 'outcrop':
                if point['height'] > MAX_OUTCROP:
                    out.error(f'cliffs {group}[{i}]: a {point["height"] / 100.0:g} m rock (the kit stops at '
                              f'{MAX_OUTCROP / 100.0:g} m)')
                continue
            wall = point.get('drop', point.get('height', 0.0))
            courses = point.get('courses')
            heights = [c['height'] for c in courses] if courses else [wall]
            counted += len(heights)
            tallest = max([tallest] + heights)
            for k, h in enumerate(heights):
                if h > MAX_COURSE + 1.0:
                    where = f'course {k}' if courses else 'its only course'
                    out.error(f'cliffs {group}[{i}]: {where} is {h / 100.0:.2f} m (at most {MAX_COURSE / 100.0:g} m)')
            if courses and abs(sum(heights) - wall) > 5.0:
                out.warn(f'cliffs {group}[{i}]: its courses add up to {sum(heights) / 100.0:.2f} m of a '
                         f'{wall / 100.0:.2f} m wall')
    stacked = sum(1 for points in data.get('cliffs', {}).values() for p in points if p.get('courses'))
    if counted:
        out.ok(f'{counted} cliff courses in {len(data.get("cliffs", {}))} groups, the tallest {tallest / 100.0:.2f} m '
               f'(at most {MAX_COURSE / 100.0:g} m); {stacked} walls stacked')
    for f in area.layout['features']:
        if f['type'] in ('plateau', 'mesa', 'pit', 'scarp') or (f['type'] == 'ridge' and f.get('cliffs') != 'none'):
            group = f.get('cliffGroup', f['id'])
            if not data.get('cliffs', {}).get(group):
                out.warn(f'{f["type"]} {f["id"]}: its cliff group {group!r} has no points')
    # Falls.
    for creek, fall in (data.get('waterfalls') or {}).items():
        if fall['dropTo'] >= fall['location'][2]:
            out.error(f'waterfall {creek}: drops to {fall["dropTo"] / 100.0:.2f} m from {fall["location"][2] / 100.0:.2f} m')
        else:
            out.ok(f'waterfall {creek}: drops {(fall["location"][2] - fall["dropTo"]) / 100.0:.1f} m')
    # The playable boundary.
    boundary = data.get('boundary')
    if area.setting == 'grounded' and boundary:
        corners, flags = boundary['corners'], boundary['openEdges']
        if len(corners) != len(flags) or not all(math.isfinite(v) for c in corners for v in c):
            out.error('boundary: its corners and open edges don\'t match')
        else:
            out.ok(f'boundary: {len(corners)} corners with ground heights, {sum(flags)} open edges, ready for '
                   'APlayableArea')
    # Open ground.
    metric = data.get('openGround')
    if metric and 'largest' in metric:
        text = (f'open ground: the largest distance to a break {metric["largest"] / 100.0:.1f} m (target '
                f'{metric["target"] / 100.0:g} m), {metric["overTarget"] * 100.0:.1f}% of {metric["walkable"]} m² '
                f'over it' + (f'; marked open on purpose: {", ".join(metric["markedOpen"])}'
                              if metric.get('markedOpen') else ''))
        if metric['largest'] > metric['target']:
            out.warn(text)
            for spot in metric.get('worst', [])[:5]:
                if spot['distance'] > metric['target']:
                    x, y = (v / 100.0 for v in spot['location'][:2])
                    print(f'             {spot["distance"] / 100.0:.1f} m at x {x:.0f}, y {y:.0f} '
                          f'(layout [{spot["location"][0]:.0f}, {spot["location"][1]:.0f}])')
        else:
            out.ok(text)


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    parser.add_argument('area')
    parser.add_argument('--layout', default='')
    parser.add_argument('--computed', default='')
    args = parser.parse_args()
    path = args.layout or area_shape.layout_path(args.area)
    out = Findings()
    print(f'Terrain check: {args.area}')
    try:
        area = area_shape.Area(path)
    except (ValueError, KeyError) as error:
        out.error(f'the layout can\'t be read: {error}')
        print('\nFAIL')
        return 1
    check_layout(area, out)
    check_computed(area, args.computed or area.computed_path, out)
    verdict = 'FAIL' if out.errors else 'PASS'
    print(f'\n{verdict}: {len(out.errors)} errors, {len(out.warnings)} warnings')
    return 1 if out.errors else 0


if __name__ == '__main__':
    sys.exit(main())
