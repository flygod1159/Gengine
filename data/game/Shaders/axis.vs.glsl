#version 450 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;

layout(location = 0) uniform mat4 u_proj;
layout(location = 1) uniform mat4 u_model;
layout(location = 2) uniform mat4 u_view;

layout(location = 0) out vec3 vColor;

void main()
{
  vColor = aColor;
  gl_Position = u_proj * u_view * u_model * vec4(aPos, 1.0);
}