#version 460 core

layout(binding = 1) uniform sampler2D u_hdrBuffer;
layout(binding = 2) uniform sampler2D u_blueNoise;
layout(location = 3) uniform bool u_useDithering = true;
layout(location = 4) uniform bool u_encodeSRGB = true;
layout(location = 5) uniform float u_exposureFactor = 1.0;

layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 fragColor;

layout(std430, binding = 0) readonly buffer exposures
{
  float readExposure;
  float writeExposure;
};

vec3 reinhard(vec3 v)
{
  return v / (1.0f + v);
}

vec3 aces_approx(vec3 v)
{
  v *= 0.6f;
  float a = 2.51f;
  float b = 0.03f;
  float c = 2.43f;
  float d = 0.59f;
  float e = 0.14f;
  return clamp((v * (a * v + b)) / (v * (c * v + d) + e), 0.0f, 1.0f);
}

vec3 ACESFitted(vec3 color);

vec3 sRGB(vec3 linearColor)
{
  bvec3 cutoff = lessThan(linearColor, vec3(0.0031308));
  vec3 higher = vec3(1.055) * pow(linearColor, vec3(1.0 / 2.4)) - vec3(0.055);
  vec3 lower = linearColor * vec3(12.92);

  return mix(higher, lower, cutoff);
}

void main()
{
  vec3 hdrColor = texture(u_hdrBuffer, vTexCoord).rgb;
#if 0
  vec3 ldr = ACESFitted(hdrColor) * u_exposureFactor * readExposure;
#else
  vec3 ldr = ACESFitted(hdrColor * u_exposureFactor * readExposure);
#endif

  if (u_useDithering)
  {
    vec2 uvNoise = vTexCoord * (vec2(textureSize(u_hdrBuffer, 0)) / vec2(textureSize(u_blueNoise, 0)));
    vec3 noiseSample = texture(u_blueNoise, uvNoise).rgb;
    ldr += vec3((noiseSample - 0.5) / 256.0);
  }

  if (u_encodeSRGB)
  {
    ldr = sRGB(ldr);
  }

  fragColor = vec4(ldr, 1.0);
}

// The code in this file after this line was originally written by Stephen Hill (@self_shadow), who deserves all
// credit for coming up with this fit and implementing it. Buy him a beer next time you see him. :)
// sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
const mat3 ACESInputMat =
{
  {0.59719, 0.35458, 0.04823},
  {0.07600, 0.90834, 0.01566},
  {0.02840, 0.13383, 0.83777}
};

// ODT_SAT => XYZ => D60_2_D65 => sRGB
const mat3 ACESOutputMat =
{
  { 1.60475, -0.53108, -0.07367},
  {-0.10208,  1.10813, -0.00605},
  {-0.00327, -0.07276,  1.07602}
};

vec3 RRTAndODTFit(vec3 v)
{
  vec3 a = v * (v + 0.0245786) - 0.000090537;
  vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
  return a / b;
}

vec3 ACESFitted(vec3 color)
{
  color = color * ACESInputMat;

  // Apply RRT and ODT
  color = RRTAndODTFit(color);

  color = color * ACESOutputMat;

  // Clamp to [0, 1]
  color = clamp(color, 0.0, 1.0);

  return color;
}