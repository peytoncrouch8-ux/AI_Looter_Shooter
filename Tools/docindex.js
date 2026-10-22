// Builds the docs index: every Markdown file in the repo (git) and the working notes in Saved/ (not in git), grouped by
// what they are for, with size, last change and health flags. Writes Docs/INDEX.md (for the AI) and
// Saved/DocIndex/index.html (the page published as the "Project Library" artifact for the user).
// Usage: node Tools/docindex.js        (run from anywhere; republish the page after)
const fs = require('fs');
const path = require('path');
const { execSync } = require('child_process');

const root = path.resolve(__dirname, '..');
const sh = (cmd) => execSync(cmd, { cwd: root, encoding: 'utf8' });

// What each file or folder is for, in plain words for the user. Order of GROUPS is the page's order.
const GROUPS = [
	{ id: 'rules', name: 'Rules', blurb: 'How we work. Loaded with every request.' },
	{ id: 'core', name: 'Read-first docs', blurb: 'Where things stand, how things are made, and the code map.' },
	{ id: 'plan', name: 'Plans and numbers', blurb: 'Phases, performance, progression, release.' },
	{ id: 'story', name: 'Story and world', blurb: 'The Revenant saga and the areas built so far.' },
	{ id: 'art', name: 'Art and sound sources', blurb: 'How models, textures and sounds are made, and their licences.' },
	{ id: 'tools', name: 'Tools', blurb: 'Readmes for standalone tools.' },
	{ id: 'design', name: 'Design studies', blurb: 'Comparisons, reworks and style research kept for reference.' },
	{ id: 'work', name: 'Working notes (Saved/, not in git)', blurb: 'Contracts, plans and holding-tree notes for agent jobs. Lost if Saved/ is wiped.' },
	{ id: 'history', name: 'History (archive)', blurb: 'Old text moved out of the living docs. Read only when needed.' },
];

const KNOWN = {
	'CLAUDE.md': ['rules', 'The project rules: your rules, build/test/perf steps, code, UI and asset rules.'],
	'Docs/Handoff.md': ['core', 'Where the work stands: your decisions, what is done, what is next, overnight calls for your review.'],
	'Docs/Pipeline.md': ['core', 'How everything is made: tools, workflows, every system\'s notes, bugs and fixes, lessons.'],
	'CODEMAP.md': ['core', 'Every source file in one line each.'],
	'Docs/Plan.md': ['plan', 'The phases: cleanup (done), cloud ideas, and the core-loop freeze (12.x: gunplay, enemies, loot, story, art).'],
	'Docs/Performance.md': ['plan', 'Frame-time measurements, before and after each change.'],
	'Docs/Progression.md': ['plan', 'XP curve, kill and mission XP, creature levels, economy.'],
	'Docs/Release.md': ['plan', 'What to do before a Steam release.'],
	'Docs/Story.md': ['story', 'The Revenant saga: world, heroes, villain, regions, plot, the creature roster.'],
	'Docs/Areas/RansomsRest.md': ['story', 'Ransom\'s Rest, area 1: layout, missions, fights, how it was built.'],
	'Docs/Areas/RansomsRest_Expansion.md': ['story', 'The mountain and mine expansion of Ransom\'s Rest.'],
	'Docs/Areas/RansomsRest_Review.md': ['design', 'A bundled review of Ransom\'s Rest (findings and fixes).'],
	'Docs/TutorialIsland.md': ['story', 'The tutorial island (now the menu backdrop): its art bible and layout.'],
	'Docs/TutorialIslandConcepts.md': ['design', 'Four layout concepts for the island (Crossroads Town was picked).'],
	'Docs/Handoffs/CloudIslandConcepts_2026-10-05.md': ['design', 'The cloud session\'s six decisions (HUD, gun ideas, embers, heroes...).'],
	'Docs/Polish/BorderlandsComparison.md': ['design', 'Borderlands vs. Revenant: what to build to stop feeling empty.'],
	'Docs/Polish/TutorialRework.md': ['design', 'Skyreach\'s tutorial rework design.'],
	'Docs/Art/StyleExploration/README.md': ['design', 'The first art-style exploration (2026-10-02).'],
	'Docs/Art/StyleLab/Briefs.md': ['design', 'The ten Style Lab art briefs (trials ended: you kept today\'s look).'],
	'Docs/Art/StyleLab/Catalog.md': ['design', 'The Style Lab catalog with Unreal recipes for each style.'],
	'Docs/Art/StyleLab/Research.md': ['design', 'Art-style research behind the Style Lab.'],
	'Art/README.md': ['art', 'Rules for Blender models and importing them.'],
	'Art/Backlog/README.md': ['art', 'Models kept out of the game for now.'],
	'Art/Sounds/README.md': ['art', 'How sounds are made: recipes, rendering, the check.'],
	'Art/Sounds/packs/README.md': ['art', 'The recorded gun/impact sounds we use, their sources and licence.'],
	'Art/ThirdParty/MakeHuman/LICENSE.ASSETS.md': ['art', 'Licence for the MakeHuman base body (CC0).'],
	'Tools/ConceptViewer/README.md': ['tools', 'The island concept viewer.'],
	'Tools/StyleLab/README.md': ['tools', 'The Style Lab tool.'],
	'Tools/StyleLab/export/EXPORT_README.md': ['tools', 'Style Lab export notes.'],
};

function groupOf(p) {
	if (KNOWN[p]) return KNOWN[p][0];
	if (p.startsWith('Docs/Archive/')) return 'history';
	if (p.startsWith('Saved/')) return 'work';
	if (p.startsWith('Tools/')) return 'tools';
	if (p.startsWith('Art/')) return 'art';
	return 'design';
}

function firstLine(p) {
	const text = fs.readFileSync(path.join(root, p), 'utf8');
	const line = text.split(/\r?\n/).find((l) => l.trim().length) || '';
	return line.replace(/^[#>*\s]+/, '').replace(/\*\*/g, '').trim();
}

function lastChange(p, tracked) {
	if (tracked) {
		const d = sh(`git log -1 --format=%cs -- "${p}"`).trim();
		if (d) return d;
	}
	return fs.statSync(path.join(root, p)).mtime.toISOString().slice(0, 10);
}

function walk(dir, out) {
	for (const e of fs.readdirSync(dir, { withFileTypes: true })) {
		const full = path.join(dir, e.name);
		if (e.isDirectory()) {
			if (!/node_modules|SoundPacks|Screenshots|ArtPreviews|Profiling|Logs/.test(e.name)) walk(full, out);
		} else if (e.name.endsWith('.md')) out.push(path.relative(root, full).replace(/\\/g, '/'));
	}
	return out;
}

const tracked = sh('git ls-files "*.md"').split(/\r?\n/).filter((p) => p && !p.includes('node_modules'));
const saved = fs.existsSync(path.join(root, 'Saved')) ? walk(path.join(root, 'Saved'), []) : [];
const files = [...tracked.map((p) => ({ p, tracked: true })), ...saved.map((p) => ({ p, tracked: false }))].map(({ p, tracked: t }) => {
	const lines = fs.readFileSync(path.join(root, p), 'utf8').split(/\r?\n/).length;
	const flags = [];
	if (t && lines > 1000 && groupOf(p) !== 'history' && groupOf(p) !== 'design') flags.push('long: split or trim');
	if (!t && /CONTRACT|PLAN|DESIGN|WIRING/.test(p)) flags.push('decisions outside git');
	return { path: p, group: groupOf(p), about: KNOWN[p] ? KNOWN[p][1] : firstLine(p), lines, changed: lastChange(p, t), tracked: t, flags };
});

const generated = new Date().toISOString().slice(0, 16).replace('T', ' ');

// --- Docs/INDEX.md (terse, for the AI) ---
let md = `# Docs index\n\nGenerated by \`node Tools/docindex.js\` (${generated}); don't edit by hand. Every Markdown file by role, with lines and last change. Working notes in Saved/ are not in git.\n`;
for (const g of GROUPS) {
	const list = files.filter((f) => f.group === g.id);
	if (!list.length) continue;
	md += `\n## ${g.name}\n`;
	for (const f of list) md += `- \`${f.path}\` (${f.lines}, ${f.changed})${f.flags.length ? ' **' + f.flags.join('; ') + '**' : ''}: ${f.about}\n`;
}
fs.writeFileSync(path.join(root, 'Docs/INDEX.md'), md);

// --- The page ---
const data = JSON.stringify({ generated, groups: GROUPS, files });
const template = fs.readFileSync(path.join(__dirname, 'docindex.template.html'), 'utf8');
fs.mkdirSync(path.join(root, 'Saved/DocIndex'), { recursive: true });
fs.writeFileSync(path.join(root, 'Saved/DocIndex/index.html'), template.replace('/*DATA*/null', data.replace(/</g, '\\u003c')));
console.log(`DOCINDEX ${files.length} files (${tracked.length} in git, ${saved.length} in Saved/) -> Docs/INDEX.md, Saved/DocIndex/index.html`);
