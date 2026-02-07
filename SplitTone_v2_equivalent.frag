// Optional: GLSL fragment shader equivalent of the DCTL logic.
// Useful if you're implementing an OpenGL-accelerated OFX plugin.
//
// Inputs:
//   vec4 inColor (linear float)
//   ivec2 pixCoord, ivec2 imageSize
// Uniforms:
//   int presetIndex;
//   float preserveMidgray;
//   vec3 pShadow;    // (p1,p2,p3)
//   vec3 pHighlight; // (p4,p5,p6)
//   bool showCurve;
//
// Note: this shader assumes 0..1 normalized coords for overlay computations.

float clampf(float v, float lo, float hi) { return max(lo, min(hi, v)); }

float getMiddleGray(int preset) {
  float mg[20] = float[20](
    0.180, 0.413, 0.413, 0.391, 0.278,
    0.383, 0.312, 0.387, 0.330, 0.336,
    0.398, 0.459, 0.391, 0.458, 0.489,
    0.363, 0.423, 0.333, 0.410, 0.488
  );
  return mg[clamp(preset, 0, 19)];
}

float applyCurve(float x, float shadowEnd, float highlightStart, float pShadow, float pHighlight) {
  x = max(0.0, x);

  if (x <= shadowEnd) {
    if (shadowEnd > 0.0) {
      float ratio = clampf(x / shadowEnd, 0.0, 1.0);
      return shadowEnd * pow(ratio, pShadow);
    }
    return x;
  } else if (x <= highlightStart) {
    return x;
  } else if (x <= 1.0) {
    float range = 1.0 - highlightStart;
    if (range > 0.0) {
      float ratio = clampf((x - highlightStart) / range, 0.0, 1.0);
      return highlightStart + range * pow(ratio, pHighlight);
    }
    return x;
  }
  return x;
}

// main() style usage:
// vec4 outColor = inColor;
// float midGray = getMiddleGray(presetIndex);
// float gapDist = midGray * preserveMidgray;
// float shadowEnd = max(0.0, midGray - gapDist);
// float highlightStart = min(1.0, midGray + gapDist);
//
// outColor.rgb = vec3(
//   applyCurve(inColor.r, shadowEnd, highlightStart, pShadow.r, pHighlight.r),
//   applyCurve(inColor.g, shadowEnd, highlightStart, pShadow.g, pHighlight.g),
//   applyCurve(inColor.b, shadowEnd, highlightStart, pShadow.b, pHighlight.b)
// );
//
// if (showCurve) { ... overlay logic ... }
