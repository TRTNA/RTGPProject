#version 410 core

const float PI = 3.14159265359;

// output shader variable
out vec4 colorFrag;


// light incidence directions (calculated in vertex shader, interpolated by rasterization)
in vec3 lightDir;
// the transformed normal has been calculated per-vertex in the vertex shader
in vec3 vNormal;
// vector from fragment to camera (in view coordinate)
in vec3 vViewPosition;

// interpolated texture coordinates
in vec2 interp_UV;

// for the correct rendering of the shadows, we need to calculate the vertex coordinates also in "light coordinates" (= using light as a camera)
in vec4 posLightSpace;