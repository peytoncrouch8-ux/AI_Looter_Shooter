// Runtime: paint the key art, wire the hover layer, and run the Skyreach model and its story switcher. Everything
// readable is already in the page; this only adds pictures and interaction.
(() => {
  // Key art on every canvas, redrawn when its size changes.
  const ro = new ResizeObserver((entries) => {
    for (const e of entries) {
      try { Art.render(e.target, e.target.dataset.art); } catch (err) { console.warn('art', err); }
    }
  });
  document.querySelectorAll('canvas[data-art]').forEach((c) => ro.observe(c));

  Tip.bind(document);
  wireCrosshair(document);

  // Each beat in the list lights its marker on the plotline, and the other way round.
  document.querySelectorAll('.story').forEach((story) => {
    const marks = story.querySelectorAll('.plotline .beat');
    story.querySelectorAll('.beats li').forEach((li) => {
      const g = marks[+li.dataset.beat];
      const on = (v) => { li.classList.toggle('on', v); if (g) g.classList.toggle('on', v); };
      li.addEventListener('pointerenter', () => on(true));
      li.addEventListener('pointerleave', () => on(false));
      if (g) {
        g.addEventListener('pointerenter', () => on(true));
        g.addEventListener('pointerleave', () => on(false));
        g.addEventListener('focus', () => on(true));
        g.addEventListener('blur', () => on(false));
      }
    });
  });

  // The Skyreach model: 3D when WebGL is there, the plan otherwise.
  const model = document.getElementById('model');
  const plan = document.getElementById('plan');
  const canvas = model.querySelector('canvas');
  const tools = model.querySelector('.model-tools');
  const pins = model.querySelector('.pins');
  const note = model.querySelector('.model-note');
  const segs = model.querySelectorAll('.seg button');
  let has3d = false;
  function setView(v) {
    const p = v === 'plan' || !has3d;
    plan.hidden = !p;
    canvas.hidden = p;
    pins.hidden = p;
    tools.hidden = p;
    note.textContent = p ? 'Plan view, north up · built from layout.json' : 'Drag to turn · built from layout.json';
    segs.forEach((b) => b.setAttribute('aria-pressed', String((b.dataset.view === 'plan') === p)));
  }
  segs.forEach((b) => b.addEventListener('click', () => setView(b.dataset.view)));
  function start3d() {
    if (has3d || !window.THREE) { setView('plan'); return; }
    try {
      Island.mount3d(model);
      has3d = true;
      setView('3d');
    } catch (err) {
      console.warn('3D view unavailable', err);
      setView('plan');
    }
  }
  if (window.THREE) start3d();
  else {
    const s = document.createElement('script');
    s.src = 'https://cdn.jsdelivr.net/npm/three@0.128.0/build/three.min.js';
    s.onload = start3d;
    s.onerror = () => setView('plan');
    document.head.appendChild(s);
    setView('plan');
  }

  // Story switcher: the tutorial's seven steps, as built or in a story's words.
  const steps = document.getElementById('steps');
  const buttons = document.querySelectorAll('.switch button');
  function highlight(step, v) {
    model.querySelectorAll(`.pin[data-step="${step}"], .p-pin[data-step="${step}"]`).forEach((n) => n.classList.toggle('on', v));
  }
  function select(id) {
    const list = STEPS[id];
    buttons.forEach((b) => b.setAttribute('aria-pressed', String(b.dataset.story === id)));
    steps.innerHTML = list.map((s, i) => `<li class="${s.pin ? 'has-pin' : ''}" data-step="${i + 1}"><span class="n">${i + 1}</span><div><div class="where">${s.where}</div>${s.label ? `<b>${s.label}</b> ` : ''}<p>${s.html}</p></div></li>`).join('');
    steps.querySelectorAll('li').forEach((li) => {
      li.addEventListener('pointerenter', () => { li.classList.add('on'); highlight(li.dataset.step, true); });
      li.addEventListener('pointerleave', () => { li.classList.remove('on'); highlight(li.dataset.step, false); });
    });
  }
  buttons.forEach((b) => b.addEventListener('click', () => select(b.dataset.story)));
  select('built');
})();
