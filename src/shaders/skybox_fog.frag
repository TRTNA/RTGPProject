#version 410 core

out vec4 colorFrag;

const float E = 2.71828182845904523536;

uniform vec3 fogColor;
uniform float fogDensity;
uniform samplerCube tCube;
in vec3 interp_UVW;

void main() {
    vec3 skyboxColor = vec3(texture(tCube, interp_UVW));
    float depth = gl_FragCoord.z;
    float fogFactor = pow(E, -fogDensity * depth);
    colorFrag = vec4(fogFactor * skyboxColor + (1.0 - fogFactor) * fogColor, 1.0);
}