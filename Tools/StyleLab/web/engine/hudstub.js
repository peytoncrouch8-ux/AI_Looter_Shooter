// A stand-in HUD with the real one's API (web/hud/hud.js), used only when that module fails to load.
export function createStubHud(container) {
  const root = document.createElement('div');
  root.style.cssText = 'position:absolute;inset:0;pointer-events:none;font:600 18px "Chakra Petch",system-ui,sans-serif;color:#e8edf2;text-shadow:0 1px 2px #000';
  root.innerHTML = '<div data-k="hp" style="position:absolute;left:3%;bottom:5%"></div><div data-k="mag" style="position:absolute;right:3%;bottom:5%;font-size:34px"></div>'
    + '<div style="position:absolute;left:50%;top:50%;width:4px;height:4px;margin:-2px;background:#fff;border-radius:2px"></div>';
  container.appendChild(root);
  const q = (k) => root.querySelector(`[data-k="${k}"]`);
  return {
    ready: Promise.resolve(), stub: true,
    resize() {}, setVisible(v) { root.style.display = v ? '' : 'none'; },
    update(dt, s = {}) { q('hp').textContent = `HP ${Math.round(s.health ?? 100)} / ${s.maxHealth ?? 100}`; q('mag').textContent = `${s.mag ?? 0} / ${s.reserve ?? 0}`; },
    onFire() {}, onHitMarker() {}, onPlayerHit() {}, onHeal() {}, onXp() {}, onLevelUp() {}, onPickup() {},
    setMission() {}, setArea() {}, setMinimap() {}, setBoss() {}, freeze() {}, unfreeze() {}, step() {},
  };
}
