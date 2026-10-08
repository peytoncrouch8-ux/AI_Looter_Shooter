// The six viewpoints every style is judged from (Art/Levels/RansomsRest/views.json), with what each still shows.
// Positions are UE cm (X north, Y east); height is above the ground under the eye; fov is UE's horizontal angle.
export const SHOTS = [
  // street: 6 m east of views.json's MainStreet, so the town gate's sign is overhead and the street reads
  { id: 'street', name: 'Main Street', X: 60, Y: -1000, height: 170, yaw: 90, pitch: -1, fov: 80, hud: true, place: 'MAIN STREET',
    spiders: [{ f: 11, r: 1.2, state: 'leap', leap: 0.42, turn: 8 }, { f: 17, r: -2.6, state: 'chase', phase: 1.1, turn: -12 },
      { f: 24.5, r: 3.4, state: 'chase', phase: 2.6, turn: 20 }],
    hudState: { health: 100, mag: 24 } },
  { id: 'fight', name: 'Street fight', X: 200, Y: 1700, height: 165, yaw: 95, pitch: -3, fov: 80, hud: true, place: 'MAIN STREET',
    spiders: [{ f: 4.2, r: 0.25, state: 'leap', leap: 0.5, turn: 0 }, { f: 13, r: -3.5, state: 'chase', phase: 0.7, turn: -25 }],
    fight: true, hudState: { health: 62, mag: 9 } },
  // chapel: at the chapel yard's gate (3.5 m north of views.json's ChapelOverTown, inside the yard's NoTrees box), over the town
  { id: 'chapel', name: 'Chapel knoll', X: 5650, Y: -2000, height: 170, yaw: 132, pitch: -4, fov: 80, hud: true, place: 'CHAPEL KNOLL',
    spiders: [], hudState: { health: 100, mag: 24 } },
  // boothill: 6 m east of views.json's ChapelFromBootHill, among the graves and clear of the oaks beside the path
  { id: 'boothill', name: 'Boot hill', X: 3400, Y: 300, height: 170, yaw: 326, pitch: 4, fov: 80, hud: true, place: 'BOOT HILL',
    spiders: [{ f: 19, r: 5, state: 'wander', phase: 0.4, turn: 70, speed: 1 }], hudState: { health: 100, mag: 24 } },
  // sink: on the pit floor between views.json's SinkRim and SinkFloor, looking across to the lit wall and the den's
  // webbed mouth (the rim view faced a wall in shadow)
  { id: 'sink', name: 'The Sink', X: 4100, Y: 5300, height: 170, yaw: 45, pitch: -4, fov: 80, hud: true, place: 'THE SINK',
    spiders: [{ f: 13, r: -1.5, state: 'chase', phase: 0.3, turn: 20 }, { f: 19, r: 4, state: 'wander', phase: 1.8, turn: -60, speed: 1 },
      { f: 25, r: -5, state: 'wander', phase: 2.9, turn: 110, speed: 1 }], hudState: { health: 100, mag: 24 } },
  { id: 'aerial', name: 'Overview', X: -6000, Y: -6000, height: 4000, yaw: 45, pitch: -22, fov: 70, hud: false, place: '',
    spiders: [], hudState: { health: 100, mag: 24 }, shadow: { near: 30, far: 460, lambda: 0.55 } },
];

export const MISSION = {
  title: 'COME HOME DEAD', step: 2, steps: 5, objective: 'Clear the spiders off Main Street', count: 3, need: 8,
  hintKey: 'F', hintText: 'Search the undertaker’s',
};
