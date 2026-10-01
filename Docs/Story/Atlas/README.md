# Story Atlas

`StoryAtlas.html` is the visual companion to the eight story directions in `Docs/Story/`: key art for each story,
diagrams of the mechanism each one turns on, plotlines by level, the legendaries, a comparison chart, and a 3D model
of the tutorial island with every story's tutorial beats pinned on it. Open it in a browser. The 3D view loads
three.js from a CDN and shows the plan view when it can't.

It is generated, so don't edit it by hand. After changing a story doc or the island layout, rebuild it:

```
cd Docs/Story/Atlas
npm install
node build.mjs
```

- `build.mjs` reads `Docs/Story/*.md` (areas, acts, legendaries, cast lines, tutorial steps, the full text) and
  `Art/Levels/TutorialIsland/layout.json` and `layout_computed.json`, draws every chart and diagram into static SVG,
  and inlines the scripts and styles.
- `src/data.mjs` holds what the docs don't: each story's plotline beats (level, stakes), the comparison ratings, the
  wanted posters and the captions.
- `src/art.js` paints the key art on canvas; `src/charts.js` and `src/diagrams.js` draw the SVG figures;
  `src/island3d.js` builds the island model and its plan view; `src/main.js` wires the page up.
