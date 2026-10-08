// The page's own interface in the LooterUI look: the style bar (top centre), the shot picker, the HUD toggle, the
// style card (I), the loading screen and the key help. Everything folds away for clean screenshots.
const $ = (s) => document.querySelector(s);
const esc = (s) => String(s ?? '').replace(/[&<>"]/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]));

export class UI {
  constructor(handlers) {
    this.h = handlers;
    this.bar = $('#stylebar');
    this.shotSel = $('#shot');
    this.card = $('#card');
    this.loading = $('#loading');
    $('#shotPrev').addEventListener('click', () => this.h.shotStep(-1));
    $('#shotNext').addEventListener('click', () => this.h.shotStep(1));
    this.shotSel.addEventListener('change', () => this.h.setShot(this.shotSel.value));
    $('#hudToggle').addEventListener('click', () => this.h.toggleHud());
    $('#cardToggle').addEventListener('click', () => this.h.toggleCard());
    $('#fold').addEventListener('click', () => this.setFolded(!this.folded));
    $('#cardClose').addEventListener('click', () => this.showCard(false));
    for (const el of document.querySelectorAll('#ui button, #ui select')) {
      el.addEventListener('keydown', (e) => { if (e.code === 'Space') e.preventDefault(); });
    }
    this.folded = false;
  }

  setStyles(list, active) {
    this.bar.innerHTML = '';
    for (const s of list) {
      const b = document.createElement('button');
      b.className = 'chip'; b.type = 'button'; b.dataset.n = s.number;
      b.title = `${s.name} (${s.family || ''}) - key ${s.number === 0 ? '`' : s.number === 10 ? '0' : s.number}`;
      b.innerHTML = `<b>${s.number}</b><span>${esc(s.name)}</span>`;
      b.addEventListener('click', () => this.h.setStyle(s.number));
      this.bar.appendChild(b);
    }
    this.setActive(active);
  }

  setActive(n, style) {
    for (const b of this.bar.children) b.classList.toggle('on', +b.dataset.n === n);
    const on = this.bar.querySelector('.chip.on');
    if (on && on.scrollIntoView) on.scrollIntoView({ block: 'nearest', inline: 'center' });
    if (style) { $('#family').textContent = style.family || ''; this.fillCard(style); }
  }

  setShots(shots, active) {
    this.shotSel.innerHTML = shots.map((s) => `<option value="${s.id}">${esc(s.name)}</option>`).join('');
    this.shotSel.value = active;
  }
  setShot(id) { this.shotSel.value = id; }
  setHud(on) { $('#hudToggle').classList.toggle('on', on); }

  fillCard(s) {
    const u = s.unreal || {};
    const li = (a) => (a || []).map((x) => `<li>${esc(x)}</li>`).join('');
    const refs = (s.refs || []).map((r) => `<li><a href="${esc(r.url)}" target="_blank" rel="noopener">${esc(r.title)}</a>${r.note ? ` <span class="note">${esc(r.note)}</span>` : ''}</li>`).join('');
    $('#cardBody').innerHTML = `
      <div class="card-head"><span class="num">${s.number}</span><div><h2>${esc(s.name)}</h2><span class="tag">${esc(s.family || '')}</span></div></div>
      <p class="tagline">${esc(s.tagline || '')}</p>
      <p>${esc(s.pitch || '')}</p>
      ${s.hudFit ? `<h3>Under the HUD</h3><p>${esc(s.hudFit)}</p>` : ''}
      ${refs ? `<h3>References</h3><ul class="refs">${refs}</ul>` : ''}
      ${u.recipe ? `<h3>In Unreal 5.8 (Medium, no Lumen or Nanite)</h3><ol>${li(u.recipe)}</ol>` : ''}
      ${u.costMs || u.risks ? `<div class="cost"><div><span class="k">GPU cost</span><span class="v">${esc(u.costMs || '?')}</span></div>${u.risks ? `<div class="risks"><span class="k">Risks</span><ul>${li(u.risks)}</ul></div>` : ''}</div>` : ''}`;
  }

  showCard(on) { this.card.hidden = !on; $('#cardToggle').classList.toggle('on', on); }
  get cardShown() { return !this.card.hidden; }

  setFolded(f) {
    this.folded = f;
    document.querySelector('#ui').classList.toggle('folded', f);
    $('#fold').textContent = f ? '▾' : '▴';
    $('#fold').setAttribute('aria-label', f ? 'Show the style bar' : 'Fold the style bar');
  }
  setVisible(on) { document.querySelector('#ui').hidden = !on; }

  progress(fraction, label) {
    $('#loadbar').style.width = (Math.max(0.02, Math.min(1, fraction)) * 100).toFixed(1) + '%';
    if (label) $('#loadtext').textContent = label;
  }
  doneLoading() { this.loading.classList.add('gone'); setTimeout(() => { this.loading.hidden = true; }, 600); }
  error(msg) { $('#loadtext').textContent = msg; this.loading.classList.add('err'); }
  toast(text) {
    const t = $('#toast');
    t.textContent = text; t.classList.add('show');
    clearTimeout(this._tt); this._tt = setTimeout(() => t.classList.remove('show'), 1600);
  }
  setLocked(locked) { document.querySelector('#lab').classList.toggle('locked', locked); }
  stats(text) { const s = $('#stats'); if (s) s.textContent = text; }
}
