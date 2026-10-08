// The style files, 00 (today's look) to 10. assemble.py and test/serve.mjs rewrite FILES from the files on disk, so a
// new NN_<id>.js is picked up without editing this list; a file that is missing or fails to load is skipped.
export const FILES = [/*FILES*/'00_today.js'/*END*/];

export async function loadStyles(onError) {
  const out = [];
  await Promise.all(FILES.map(async (f) => {
    try {
      const m = await import('./' + f);
      const s = m.default;
      if (!s || typeof s !== 'object') throw new Error('no default export');
      if (typeof s.number !== 'number') s.number = parseInt(f, 10) || 0;
      s.file = f;
      out.push(s);
    } catch (e) {
      if (onError) onError(f, e);
    }
  }));
  // one style per number (the first file wins)
  const seen = new Set();
  return out.sort((a, b) => a.number - b.number || a.file.localeCompare(b.file)).filter((s) => !seen.has(s.number) && seen.add(s.number));
}
