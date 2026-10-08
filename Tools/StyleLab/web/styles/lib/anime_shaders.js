// Skyward Anime's material hooks. They run after the toon ramp on the lit colour (see the Style Lab README).

// Grass and leaves: a vertical gradient from deep green at the root to bright yellow-green tips (the foliage's vertex
// red is its wind weight, 0 at the root and 1 at the tip), plus a sunlit sheen on the tips, the way an anime
// background painter lays in a meadow.
export function foliageHook({ root = [0.48, 0.62, 0.58], tip = [1.16, 1.12, 0.72], amount = 1.0 } = {}) {
  const v = (a) => `vec3(${a.map((x) => x.toFixed(3)).join(', ')})`;
  return /* glsl */`
  float agT = clamp(vSlVC.r, 0.0, 1.0);
  agT = max(agT, smoothstep(-0.2, 0.9, N.y) * 0.55);
  col *= mix(vec3(1.0), mix(${v(root)}, ${v(tip)}, agT), ${amount.toFixed(3)});
  col += diffuseColor.rgb * slSunCol * 0.06 * agT * shadow * step(0.0, NdL);
  `;
}

// The ground: lit grass glows yellow-green, dirt goes warm ochre, and far ground takes a painted wash.
export function groundHook() {
  return /* glsl */`
  float gG = (1.0 - clamp(tDirt, 0.0, 1.0)) * (1.0 - clamp(max(tRock, tSteep), 0.0, 1.0));
  float gLit = shadow * step(0.0, NdL);
  col *= mix(vec3(1.0), vec3(1.0, 1.08, 0.82), gG * gLit * 0.6);
  col *= mix(vec3(1.0), vec3(1.08, 0.98, 0.86), clamp(tDirt, 0.0, 1.0) * gLit * 0.5);
  // big soft painted patches, so a field is never one flat colour
  float gP = slFbm(worldPos.xz * 0.03) - 0.5;
  col *= 1.0 + gP * 0.12;
  // the ground's own light and dark laid in as flat cel shapes, the way a background painter blocks a street
  float gN1 = slFbm(worldPos.xz * 0.32 + 3.1) + (slVNoise(worldPos.xz * 2.1) - 0.5) * 0.08;
  float gN2 = slFbm(worldPos.xz * 0.9 + 11.7);
  float gDist = length(worldPos - cameraPosition);
  float gK = 1.0 - smoothstep(60.0, 160.0, gDist);
  float gDark = smoothstep(0.585, 0.6, gN1) * gK;
  float gLight = smoothstep(0.64, 0.655, gN2) * gK * (1.0 - gDark);
  col *= mix(vec3(1.0), vec3(0.86, 0.84, 0.9), gDark);
  col *= mix(vec3(1.0), vec3(1.07, 1.05, 0.98), gLight);
  `;
}

// A soft sky-coloured bounce on everything facing up in the shade (anime backgrounds keep shadows airy).
export const AIRY = /* glsl */`
  col += diffuseColor.rgb * vec3(0.34, 0.36, 0.60) * (1.0 - shadow * step(0.0, NdL)) * clamp(N.y * 0.5 + 0.6, 0.0, 1.0) * 0.5 * ao;
`;
