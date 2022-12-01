#version 410 core

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

out vec3 wNormal;
out vec3 wPos;

out vec2 interp_UV;
out vec3 interp_UVW;

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 UV;

void main() {
    vec4 mPosition = modelMatrix * vec4(position, 1.0);
    vec4 mvPosition = viewMatrix * mPosition;
    
    wPos = mPosition.xyz;
    wNormal = vec3(modelMatrix * vec4(normal, 0.0));

    interp_UV = UV;
    interp_UVW = position;

    gl_Position = projectionMatrix * mvPosition;
}