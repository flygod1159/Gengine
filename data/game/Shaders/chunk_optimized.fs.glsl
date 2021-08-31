#version 460 core

// material properties
layout(location = 0) in vec3 vPos;
layout(location = 1) in vec3 vTexCoord;
layout(location = 2) in vec4 vLighting; // RGBSun
layout(location = 3) in flat uint vQuadAO;

layout(location = 1) uniform vec3 viewPos;  // world space
layout(location = 2) uniform vec3 u_envColor = vec3(1.0);
layout(location = 3) uniform float u_minBrightness = 0.01;
layout(location = 4, binding = 0) uniform sampler2DArray textures;
layout(location = 5) uniform float u_ambientOcclusionStrength = 0.5;

layout(location = 0) out vec4 fragColor;

// dithered transparency
bool clipTransparency(float alpha)
{
  int x = (int(gl_FragCoord.x)) % 4;
  int y = (int(gl_FragCoord.y)) % 4;

  const mat4 thresholdMatrix = mat4
  (
    1.0 / 16.0,  9.0 / 16.0,  3.0 / 16.0, 11.0 / 16.0,
    13.0 / 16.0,  5.0 / 16.0, 15.0 / 16.0,  7.0 / 16.0,
    4.0 / 16.0, 12.0 / 16.0,  2.0 / 16.0, 10.0 / 16.0,
    16.0 / 16.0,  8.0 / 16.0, 14.0 / 16.0,  6.0 / 16.0
  );

  float limit = thresholdMatrix[x][y];

  // Is this pixel below the opacity limit? Skip drawing it
  return alpha < limit;
}

vec3 GetNormal()
{
  vec3 dudx = dFdx(vPos);
  vec3 dudy = dFdy(vPos);
  return normalize(cross(dudx, dudy));
}

float CalculateAO()
{
  // bilinearly interpolate AO across face
  uint ao1i = (vQuadAO >> 0) & 0x3;
  uint ao2i = (vQuadAO >> 2) & 0x3;
  uint ao3i = (vQuadAO >> 4) & 0x3;
  uint ao4i = (vQuadAO >> 6) & 0x3;
  float ao1f = float(ao1i) / 3.0;
  float ao2f = float(ao2i) / 3.0;
  float ao3f = float(ao3i) / 3.0;
  float ao4f = float(ao4i) / 3.0;

  float r = mix(ao1f, ao2f, vTexCoord.y);
  float l = mix(ao4f, ao3f, vTexCoord.y);
  float v = mix(l, r, vTexCoord.x);
  //float edgeFactor = 
  return v;
}

void main()
{
  vec4 texColor = texture(textures, vTexCoord).rgba;

  // dithering
  if ((texColor.a < 1 && clipTransparency(texColor.a) || texColor.a == 0))
  {
    discard;
  }

  vec3 diffuse = texColor.rgb;
  vec3 envLight = vLighting.a * u_envColor;
  vec3 shaded = diffuse * max(vLighting.rgb, envLight);
  shaded = max(shaded, vec3(u_minBrightness));
  shaded = mix(shaded, shaded * CalculateAO(), u_ambientOcclusionStrength);

  fragColor = vec4(shaded, 1.0);
}