// The HUD's DOM, ported from Docs/HudMockup/NewHud.dc.html: its markup and SVG shapes as they are, with the dynamic
// parts marked data-r="name" (hud.js collects them). Classes carry an "lh-" prefix and every id is prefixed too, so the
// host page and the HUD can't touch each other. Text sizes in SVG are the mockup's x 4/3 (the game's text is a third
// bigger than the mockup's px); the minimap's waypoint, the 120 FPS label and the mockup's demo controls are left out.
import { BUST, EYES } from './portrait.js';

const SLOT_X = 1743;
const SLOT_Y = 780;
const SLOT_STEP = 76;
const CARTRIDGE = 'M2 2 L9 2 L9 9 L15 9 L15 5 L176 5 L186 10 C206 12 222 20 230 30 C222 40 206 48 186 50 L176 55 L15 55 L15 51 L9 51 L9 58 L2 58 L0 56 L0 4 Z';

const DEFS = `
<svg width="0" height="0" style="position:absolute" aria-hidden="true"><defs>
<radialGradient id="pGlass" cx="50%" cy="40%" r="64%"><stop offset="0" stop-color="#1f5470"/><stop offset="0.5" stop-color="#0c2a3c"/><stop offset="1" stop-color="#04101a"/></radialGradient>
<radialGradient id="pEye" cx="50%" cy="50%" r="50%"><stop offset="0" stop-color="#e6fbff" stop-opacity="1"/><stop offset="0.3" stop-color="#7fd8ff" stop-opacity="0.7"/><stop offset="1" stop-color="#5ccaff" stop-opacity="0"/></radialGradient>
<pattern id="pScan" width="4" height="4" patternUnits="userSpaceOnUse"><rect width="4" height="1.2" fill="#5ac8ff" opacity="0.08"/></pattern>
<linearGradient id="gmUp" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="#76818e"/><stop offset="0.5" stop-color="#46505b"/><stop offset="1" stop-color="#2c333b"/></linearGradient>
<linearGradient id="gmDown" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="#2e353d"/><stop offset="0.6" stop-color="#1d2228"/><stop offset="1" stop-color="#111418"/></linearGradient>
<linearGradient id="gmRing" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#6b7683"/><stop offset="0.5" stop-color="#2f363f"/><stop offset="1" stop-color="#15191e"/></linearGradient>
<linearGradient id="gemFill" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#d4f4ff"/><stop offset="0.5" stop-color="#bdeeff"/><stop offset="0.5" stop-color="#4ab5ee"/><stop offset="1" stop-color="#2f9fdc"/></linearGradient>
<linearGradient id="magCyan" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#bdeaff"/><stop offset="0.4" stop-color="#a6e3ff"/><stop offset="0.4" stop-color="#43b2ec"/><stop offset="1" stop-color="#2f9ad6"/></linearGradient>
<linearGradient id="magOrange" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#ffd49a"/><stop offset="0.4" stop-color="#ffc06a"/><stop offset="0.4" stop-color="#ff9f1c"/><stop offset="1" stop-color="#e2850f"/></linearGradient>
<radialGradient id="bannerGlow" cx="50%" cy="50%" r="50%"><stop offset="0" stop-color="#9fe0ff" stop-opacity="0.55"/><stop offset="0.5" stop-color="#5ac8ff" stop-opacity="0.18"/><stop offset="1" stop-color="#5ac8ff" stop-opacity="0"/></radialGradient>
<pattern id="hatch" width="6" height="6" patternUnits="userSpaceOnUse" patternTransform="rotate(45)"><rect width="1.4" height="6" fill="#000" opacity="0.13"/></pattern>
<clipPath id="medWin"><polygon points="0,-75 75,0 0,75 -75,0"/></clipPath>
<clipPath id="magClip"><path d="M17 7.5 L175 7.5 L184 12 C202 14 216 21 223 30 C216 39 202 46 184 48 L175 52.5 L17 52.5 Z"/></clipPath>
${BUST}
${EYES}
</defs></svg>`;

const EDGES = `
<div class="lh-edges" data-r="edges"><div class="lh-edge lh-edge-l"></div><div class="lh-edge lh-edge-r"></div><div class="lh-edge lh-edge-t"></div><div class="lh-edge lh-edge-b"></div></div>`;

const FRAME = `
<div class="lh-g lh-g-bl"><div class="lh-frame">
<div class="lh-hpbar"><div class="lh-hpbez"><div class="lh-hptrack">
<div class="lh-hpchip" data-r="hpchip"></div>
<div class="lh-hpfill" data-r="hpfill"><div class="lh-hpshine" data-r="hpshine"></div></div>
<div class="lh-hpcut" style="left:25%"></div><div class="lh-hpcut" style="left:50%"></div><div class="lh-hpcut" style="left:75%"></div>
</div></div><div class="lh-hpclamp"></div></div>
<div class="lh-hpnum lh-ol" data-r="hpnum"><b data-r="hpval" style="font-weight:inherit"></b><span data-r="hpmax"></span></div>
<div class="lh-healfloat lh-ol" data-r="healfloat">+0</div>

<div class="lh-xpbar">${Array.from({ length: 10 }, (_, i) => `<div class="lh-xpsec"><div class="lh-xpfill" data-r="xpfill${i}" style="width:0%"></div><div class="lh-xpgain" data-r="xpgain${i}"></div></div>`).join('')}</div>
<div class="lh-xptext lh-ol"><b data-r="xpnow" style="font-weight:inherit"></b><span data-r="xpmax"></span></div>
<div class="lh-xpfloat lh-ol" data-r="xpfloat">+0 XP</div>

<svg class="lh-med" viewBox="-110 -110 220 220" aria-label="Portrait of your character">
<polygon points="-72,-17 -112,0 -72,17 -85,0" fill="url(#gmUp)" stroke="#0e1116" stroke-width="2.5" stroke-linejoin="round"/>
<polygon points="-84,-7 -101,0 -84,7 -89,0" fill="#ff9f1c" stroke="#0e1116" stroke-width="1.4" stroke-linejoin="round"/>
<polygon points="-86,0 0,-86 86,0" fill="url(#gmUp)"/>
<polygon points="-86,0 0,86 86,0" fill="url(#gmDown)"/>
<polygon points="0,-86 86,0 0,86 -86,0" fill="none" stroke="#0e1116" stroke-width="3.2" stroke-linejoin="round"/>
<polyline points="-81,0 0,-81 81,0" fill="none" stroke="#ffffff" stroke-opacity="0.28" stroke-width="1.3"/>
<polyline points="-81,0 0,81 81,0" fill="none" stroke="#000000" stroke-opacity="0.35" stroke-width="1.3"/>
<g clip-path="url(#medWin)">
<g class="lh-face" data-r="face"><g class="lh-breath" data-r="breath"><g transform="translate(-75 -75) scale(0.75)">
<use href="#bust"/>
<g class="lh-calmfx" data-r="calmfx" opacity="1"><g class="lh-blink" data-r="blink"><use href="#eyesOpen"/></g><use href="#browsCalm"/></g>
<g class="lh-squint" data-r="squint" opacity="0"><use href="#eyesHurt"/></g>
<g class="lh-flare" data-r="flare"><use href="#eyesFlare"/></g>
</g></g></g>
<polygon class="lh-hurtfx" data-r="hurtfx" points="0,-75 75,0 0,75 -75,0" fill="#ff3b2e"/>
<polygon class="lh-lowfx" data-r="lowfx" points="0,-75 75,0 0,75 -75,0" fill="#ff2a1e"/>
</g>
<polygon points="0,-75 75,0 0,75 -75,0" fill="none" stroke="#0e1116" stroke-width="2.6"/>
<polygon points="0,-73.2 73.2,0 0,73.2 -73.2,0" fill="none" stroke="#5ac8ff" stroke-width="1.6" opacity="0.9"/>
<path d="M-16 -80 L0 -98 L16 -80 L10 -75 L0 -87 L-10 -75 Z" fill="#ff9f1c" stroke="#0e1116" stroke-width="2.2" stroke-linejoin="round"/>
<path d="M-10 81 L0 93 L10 81 L6 78 L0 85 L-6 78 Z" fill="#ff9f1c" stroke="#0e1116" stroke-width="1.8" stroke-linejoin="round"/>
</svg>

<svg class="lh-gem" viewBox="-26 -26 52 52" aria-label="Level">
<polygon class="lh-gemring" data-r="gemring" points="0,-22 22,0 0,22 -22,0" fill="none" stroke="#9fe0ff" stroke-width="2.4"/>
<g class="lh-gemcore" data-r="gemcore">
<polygon points="0,-22 22,0 0,22 -22,0" fill="url(#gemFill)" stroke="#0a1218" stroke-width="2.8" stroke-linejoin="round"/>
<polyline points="-14.5,0 0,-14.5 14.5,0" fill="none" stroke="#ffffff" stroke-opacity="0.8" stroke-width="1.3"/>
<text data-r="gemtext" x="0" y="8.7" text-anchor="middle" fill="#0a1218" font-size="24">1</text>
</g></svg>
</div></div>`;

function slot(i) {
	return `
<div class="lh-slot lh-empty" data-r="slot${i}" style="left:${SLOT_X}px;top:${SLOT_Y + SLOT_STEP * i}px">
<div class="lh-glow lh-full"></div>
<svg width="64" height="64" viewBox="-32 -32 64 64">
<g class="lh-void"><circle r="32" fill="#0e1116" opacity="0.55"/><circle r="26" fill="rgba(7,26,40,0.6)"/><circle r="28" fill="none" stroke="#dcefff" stroke-width="2" stroke-dasharray="4.4 4.4" opacity="0.6"/></g>
<g class="lh-full"><circle r="32" fill="#0e1116"/><circle r="30" fill="url(#gmRing)"/><circle r="27.5" fill="rgba(7,26,40,0.82)"/>
<path data-r="arc${i}" d="M -20.9 17.5 A 27.25 27.25 0 0 0 20.9 17.5" fill="none" stroke="#cb7cf3" stroke-width="4.5"/>
<circle r="27.9" fill="none" stroke="#5ac8ff" stroke-width="1" opacity="0.55"/>
<circle class="lh-ring" data-r="ring${i}" r="31.6" fill="none" stroke="#ff9f1c" stroke-width="3"/></g>
</svg>
<img class="lh-gun lh-full" data-r="gun${i}" alt="" draggable="false">
<div class="lh-tab" data-r="tab${i}">${i + 1}</div>
<img class="lh-ammo lh-full" data-r="ammo${i}" alt="" draggable="false">
</div>`;
}

const WEAPONS = `
<div class="lh-g lh-g-br">${slot(0)}${slot(1)}${slot(2)}
<div class="lh-status lh-ol" data-r="status"></div>
<svg class="lh-a lh-sh" style="left:1821px;top:800px;overflow:visible" width="51" height="196" viewBox="0 0 60 230" aria-label="Magazine: rounds in it, then the reserve">
<g transform="translate(0 230) rotate(-90)">
<path d="${CARTRIDGE}" fill="rgba(5,16,24,0.76)"/>
<g clip-path="url(#magClip)">
<rect data-r="magfill" x="15" y="0" width="0" height="60" fill="url(#magCyan)"/>
<rect data-r="maghatch" x="15" y="0" width="0" height="60" fill="url(#hatch)"/>
<rect data-r="magedge" x="15" y="0" width="1.8" height="60" fill="#dcefff" opacity="0.9"/>
</g>
<g stroke="#dcefff" stroke-width="1.2" opacity="0.4"><line x1="120.5" y1="9" x2="120.5" y2="51"/><line x1="173.2" y1="9" x2="173.2" y2="51"/></g>
<path d="${CARTRIDGE}" fill="none" stroke="#0e1116" stroke-width="4.6" stroke-linejoin="round"/>
<path data-r="magline" d="${CARTRIDGE}" fill="none" stroke="#dcefff" stroke-width="1.8" stroke-linejoin="round"/>
</g>
<text data-r="magcount" class="lh-count" x="30" y="187" text-anchor="middle" fill="#ffffff" font-size="36" stroke="#00182a" stroke-width="4.5" paint-order="stroke fill" stroke-linejoin="round">0</text>
<text data-r="magreserve" class="lh-count" x="30" y="207" text-anchor="middle" fill="#cfe2ef" font-size="21.33" stroke="#00182a" stroke-width="3.6" paint-order="stroke fill" stroke-linejoin="round">/0</text>
</svg>
<div class="lh-firemode lh-ol" data-r="firemode"></div>
<div class="lh-gunname lh-ol" data-r="gunname"></div>
<div class="lh-raritygem" data-r="raritygem"></div>
</div>`;

const MINIMAP = `
<div class="lh-g lh-g-tr">
<canvas class="lh-map lh-sh" data-r="mapcanvas" width="344" height="344" aria-label="Minimap"></canvas>
<svg class="lh-a" style="left:1700px;top:24px;overflow:visible" width="196" height="196" viewBox="0 0 196 196" aria-hidden="true">
<circle cx="98" cy="98" r="90.5" fill="none" stroke="url(#gmRing)" stroke-width="9"/>
<circle cx="98" cy="98" r="95" fill="none" stroke="#0e1116" stroke-width="1.8"/>
<circle cx="98" cy="98" r="86" fill="none" stroke="#0e1116" stroke-width="1.6"/>
<circle cx="98" cy="98" r="84.6" fill="none" stroke="#5ac8ff" stroke-width="1" opacity="0.75"/>
<g data-r="mapticks" transform="translate(98 98) rotate(0)" stroke="#c9d7e2" stroke-width="1.4" opacity="0.8">
<line y1="-94" y2="-88" transform="rotate(30)"/><line y1="-94" y2="-88" transform="rotate(60)"/><line y1="-94" y2="-88" transform="rotate(120)"/><line y1="-94" y2="-88" transform="rotate(150)"/>
<line y1="-94" y2="-88" transform="rotate(210)"/><line y1="-94" y2="-88" transform="rotate(240)"/><line y1="-94" y2="-88" transform="rotate(300)"/><line y1="-94" y2="-88" transform="rotate(330)"/>
<line y1="-95" y2="-86" transform="rotate(90)" stroke-width="2.2"/><line y1="-95" y2="-86" transform="rotate(180)" stroke-width="2.2"/><line y1="-95" y2="-86" transform="rotate(270)" stroke-width="2.2"/>
</g>
<g data-r="mapn" transform="translate(98 98) rotate(0) translate(0 -90.5) rotate(0)">
<circle r="9.5" fill="#0e1116" stroke="#ff9f1c" stroke-width="1.6"/>
<text y="5.9" text-anchor="middle" fill="#ff9f1c" font-size="16">N</text>
</g>
<path d="M91 -0.5 L105 -0.5 L98 9 Z" fill="#ff9f1c" stroke="#0e1116" stroke-width="1.4" stroke-linejoin="round"/>
<path d="M98 88 L106 106 L98 101.5 L90 106 Z" fill="#ffffff" stroke="#0a1218" stroke-width="1.8" stroke-linejoin="round"/>
</svg>
<div class="lh-area lh-ol" data-r="place" style="top:224px;font-size:20px;letter-spacing:2.5px;color:#dcefff;line-height:20px"></div>
<div class="lh-area lh-ol" data-r="area" style="top:244px;font-size:14px;letter-spacing:3.5px;color:#8fb3cc;line-height:14px"></div>
</div>`;

const TRACKER = `
<div class="lh-g lh-g-tl">
<div class="lh-track" data-r="track" style="display:none">
<svg class="lh-trackroute" viewBox="0 0 48 70" aria-hidden="true">
<path d="M17 35 L17 58 L38 58" fill="none" stroke="#0a1218" stroke-width="3" stroke-opacity="0.45" stroke-linejoin="round"/>
<path d="M17 35 L17 58 L38 58" fill="none" stroke="#5ac8ff" stroke-width="1.2" stroke-opacity="0.8" stroke-linejoin="round"/>
</svg>
<svg class="lh-trackmedal" viewBox="-17 -17 34 34" aria-hidden="true">
<circle r="16" fill="#0e1116"/><circle r="14.6" fill="url(#gmRing)"/><circle r="12" fill="rgba(7,26,40,0.9)"/>
<circle r="12.4" fill="none" stroke="#5ac8ff" stroke-width="1" opacity="0.6"/>
<polygon points="0.00,-10.40 2.65,-3.64 9.89,-3.21 4.28,1.39 6.11,8.41 0.00,4.50 -6.11,8.41 -4.28,1.39 -9.89,-3.21 -2.65,-3.64" fill="#ff9f1c" stroke="#0a1218" stroke-width="1.5" stroke-linejoin="round"/>
<polyline points="-7.6,-2.6 -2.1,-2.9 0,-8.1" fill="none" stroke="#ffffff" stroke-opacity="0.55" stroke-width="1"/>
</svg>
<div class="lh-tracktitle lh-ol" data-r="tracktitle"></div>
<div class="lh-tracksteps" data-r="tracksteps"></div>
<div class="lh-trackstep lh-ol" data-r="trackstep"></div>
<div class="lh-trackobj" data-r="trackobj">
<svg viewBox="0 0 16 16" aria-hidden="true">
<polygon data-r="chevron" points="2,1.5 8,1.5 14.5,8 8,14.5 2,14.5 8.5,8" fill="#ff9f1c" stroke="#0a1218" stroke-width="1.6" stroke-linejoin="round"/>
<path data-r="tick1" d="M2.5 8.5 L6.5 12.5 L14 4" fill="none" stroke="#0a1218" stroke-width="5" stroke-linecap="round" stroke-linejoin="round" opacity="0"/>
<path data-r="tick2" d="M2.5 8.5 L6.5 12.5 L14 4" fill="none" stroke="#5ac8ff" stroke-width="2.4" stroke-linecap="round" stroke-linejoin="round" opacity="0"/>
</svg>
<span class="lh-txt lh-ol" data-r="objtext"></span>
<span class="lh-trackcount lh-ol" data-r="trackcount"></span>
</div>
<div class="lh-trackhint lh-ol" data-r="trackhint"><span class="lh-key" data-r="hintkey"></span><span data-r="hinttext"></span></div>
</div>
</div>`;

const BOSS = `
<div class="lh-g lh-g-tc">
<div class="lh-boss" data-r="boss">
<svg class="lh-a" style="left:4px;top:22px;overflow:visible" width="52" height="52" viewBox="-26 -26 52 52">
<polygon points="0,-22 22,0 0,22 -22,0" fill="#ff9f1c" stroke="#0a1218" stroke-width="2.8" stroke-linejoin="round"/>
<polyline points="-14.5,0 0,-14.5 14.5,0" fill="none" stroke="#ffffff" stroke-opacity="0.7" stroke-width="1.3"/>
<text data-r="bosslevel" x="0" y="8.7" text-anchor="middle" fill="#0a1218" font-size="24">1</text>
</svg>
<div class="lh-bossname lh-ol" data-r="bossname"></div>
<div class="lh-bossbar"><div class="lh-hpbez"><div class="lh-hptrack" style="left:4px">
<div class="lh-hpchip" data-r="bosschip"></div>
<div class="lh-hpfill" data-r="bossfill"></div>
<div class="lh-hpcut" data-r="bosscut1" style="left:70%"></div>
<div class="lh-hpcut" data-r="bosscut2" style="left:40%"></div>
</div></div><div class="lh-hpclamp" style="height:36px"></div></div>
<div class="lh-bossphase lh-ol" data-r="bossphase"></div>
</div>
</div>`;

const CENTRE = `
<div class="lh-g lh-g-c">
<div class="lh-banner" data-r="banner">
<svg class="lh-big" viewBox="-90 -90 180 180">
<circle r="80" fill="url(#bannerGlow)"/>
<g stroke="#ff9f1c" stroke-width="2" stroke-linecap="round" opacity="0.75">
<line x1="0" y1="-64" x2="0" y2="-86"/><line x1="64" y1="0" x2="86" y2="0"/><line x1="0" y1="64" x2="0" y2="86"/><line x1="-64" y1="0" x2="-86" y2="0"/>
<line x1="42" y1="-42" x2="54" y2="-54"/><line x1="42" y1="42" x2="54" y2="54"/><line x1="-42" y1="42" x2="-54" y2="54"/><line x1="-42" y1="-42" x2="-54" y2="-54"/>
</g>
<polygon points="0,-58 58,0 0,58 -58,0" fill="none" stroke="#9fe0ff" stroke-width="1.6" opacity="0.8"/>
<polygon points="0,-46 46,0 0,46 -46,0" fill="url(#gemFill)" stroke="#0a1218" stroke-width="4" stroke-linejoin="round"/>
<polyline points="-31,0 0,-31 31,0" fill="none" stroke="#ffffff" stroke-opacity="0.8" stroke-width="2"/>
<text data-r="bannerlevel" x="0" y="17.3" text-anchor="middle" fill="#0a1218" font-size="50.67">1</text>
</svg>
<div class="lh-title lh-ol"><span class="lh-rule"></span>LEVEL UP<span class="lh-rule"></span></div>
<div class="lh-sub lh-ol" data-r="bannersub">MAX HEALTH +8%</div>
</div>
<svg class="lh-xh" data-r="xh" viewBox="-20 -20 40 40" aria-hidden="true">
<g fill="#ffffff" stroke="rgba(0,39,56,0.85)" stroke-width="1" opacity="0.92">
<rect x="-2" y="-19" width="4" height="9"/><rect x="-2" y="10" width="4" height="9"/>
<rect x="-19" y="-2" width="9" height="4"/><rect x="10" y="-2" width="9" height="4"/>
</g>
<rect x="-1.5" y="-1.5" width="3" height="3" fill="#ffffff" opacity="0.85"/>
</svg>
<svg class="lh-hm" data-r="hm" viewBox="-17 -17 34 34" aria-hidden="true">
<g stroke="rgba(0,39,56,0.85)" stroke-width="1">
<rect class="lh-hmtick" x="-2" y="-17" width="4" height="11"/><rect class="lh-hmtick" x="-2" y="6" width="4" height="11"/>
<rect class="lh-hmtick" x="-17" y="-2" width="11" height="4"/><rect class="lh-hmtick" x="6" y="-2" width="11" height="4"/>
</g>
</svg>
${[0, 1, 2].map((i) => `<div class="lh-feed lh-ol" data-r="feed${i}" style="top:528px"><span class="lh-n" data-r="feedn${i}"></span><img data-r="feedicon${i}" alt="" draggable="false"><span data-r="feedname${i}"></span></div>`).join('')}
</div>`;

/** The HUD's whole DOM as one string, with every id prefixed ("lh-"). */
export function buildMarkup() {
	const html = `${EDGES}<div class="lh-stage" data-r="stage">${DEFS}${FRAME}${WEAPONS}${MINIMAP}${TRACKER}${BOSS}${CENTRE}</div>`;
	return html
		.replace(/\sid="([A-Za-z0-9_]+)"/g, ' id="lh-$1"')
		.replace(/url\(#([A-Za-z0-9_]+)\)/g, 'url(#lh-$1)')
		.replace(/href="#([A-Za-z0-9_]+)"/g, 'href="#lh-$1"');
}
