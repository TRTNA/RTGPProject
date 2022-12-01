#version 410 core

const float PI = 3.14159265359;
const int NSAMPLES = 25;
const float E = 0.5772156649;

out vec4 colorFrag;

uniform vec3 wLightPos;
uniform vec3 wCameraPos;

in vec2 interp_UV;
in vec3 interp_UVW;

uniform mat4 inverseViewProjMatrix;
uniform float width;
uniform float height;

uniform samplerCube skyboxTex;
// texture sampler for the depth map
uniform samplerCube depthMap;

uniform float far_plane;

uniform vec3 absorptionCoeff;
uniform vec3 scatteringCoeff;
uniform float g; //parameter used by Mie phase function to represent backward (g<0), isotropic (g=0) and forward (g > 0) scattering

vec3 extinctionCoeff;

vec3 sampleOffsetDirections[20] = vec3[]
(
   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
); 

subroutine float phase_function(float cosTheta);
subroutine uniform phase_function PhaseFunction;

vec3 lightRadiance(float dist) {
    vec3 cLight0 = vec3(1.0, 1.0, 1.0);
    float ro = 30.0;
    float epsilon = 0.1;
    return cLight0 * ((ro*ro) / (dist*dist + epsilon));
}

float calculateShadow(vec3 wFragPos)
{
    float shadow = 0.0;
    float bias   = 0.70;
    int samples  = 20;

    float diskRadius = 0.10; 

    vec3 lightToFrag = wFragPos - wLightPos;
    // now get current linear depth as the length between the fragment and light position
    float currentDepth = length(lightToFrag);
    
    for(int i = 0; i < samples; ++i)
    {
        float closestDepth = texture(depthMap, lightToFrag + sampleOffsetDirections[i] * diskRadius).r;
        closestDepth *= far_plane;   // undo mapping [0;1]
        if(currentDepth - bias > closestDepth)
            shadow += 1.0;
    }
    shadow /= float(samples);
    return shadow;
}


//light dir is direction of light from light to point
vec3 calculateScattering(vec3 wSamplePos, vec3 wLightDir, vec3 wViewDir) {
    float xShadowVal = calculateShadow(wSamplePos);

    return PI * PhaseFunction(dot(wViewDir, wLightDir))
                *(1.0-xShadowVal)
                *lightRadiance(length(wSamplePos-wLightPos));
}


subroutine(phase_function)
float uniformPhaseFunc(float cosTheta) {
    return 1.0 / (4.0*PI);
}

subroutine(phase_function)
float rayleighPhaseFunc(float cosTheta) {
    return (3.0/(16.0*PI))*(1.0 + cosTheta*cosTheta);
}

subroutine(phase_function)
float miePhaseFunc(float cosTheta) {
    float num = 1.0 - g*g;
    float denom = (4.0*PI)*pow((1.0 + g*g - 2.0*g*cosTheta), 1.5);
    return num/denom;
}

subroutine(phase_function) 
float schlickPhaseFunc(float cosTheta) {
    float k = 1.55*g - 0.55*g*g*g;
    float num = 1 - k*k;
    float denom = (4*PI)*pow((1+k*cosTheta), 2.0);
    return num/denom;
}

vec3 calculateTransmittance(float dist) {
    return exp(-dist*extinctionCoeff.xyz);
}

vec3 computeFragmentWorldPosition() {
    vec4 ndc = vec4(0.0, 0.0, 0.0, 0.0);

    //from screen coordinates to NDC
    ndc.x = (2.0 * gl_FragCoord.x) / width - 1.0;
    ndc.y = 1.0 - (2.0 * gl_FragCoord.y) / height;
    ndc.z = 1.0; //it is a skybox
    ndc.w = 1.0;

    //invert view projection
    vec4 wCoord = inverseViewProjMatrix * ndc;

    // invert perspective divide
    wCoord.x *= wCoord.w;
    wCoord.y *= wCoord.w;
    wCoord.z *= wCoord.w;

    return wCoord.xyz;
}

void main() {
    extinctionCoeff = absorptionCoeff + scatteringCoeff;

    vec3 fragRadiance = vec3(texture(skyboxTex, interp_UVW));
    vec3 wPos = computeFragmentWorldPosition(); 
    vec3 wCamToFrag = wPos - wCameraPos;
    float distanceFromCamera = length(wCamToFrag);
    vec3 fragCameraTransmittance = calculateTransmittance(distanceFromCamera);
    
    //1st term
    vec3 transmittedSurfaceRadiance = fragCameraTransmittance*fragRadiance;

    //Ray marching init
    // NSAMPLES + 1 because we want to esclude samples at camera pos and at fragment pos
    vec3 wStep = wCamToFrag / (NSAMPLES+1);
    float differential = length(wStep);

    // Not using random on skybox to avoid visual artifacts
    vec3 wSamplePos = wCameraPos + wStep;

    vec3 result = transmittedSurfaceRadiance;

    //Ray marching
    for(int i = 0; i < NSAMPLES; i++) {
        vec3 cameraSampleTransmittance = calculateTransmittance(length(wSamplePos));
        //for the scattering we need the direction of the light towards the fragment
        vec3 wLightToSample = normalize(wSamplePos - wLightPos);
        vec3 wSampleToCamera = normalize(wCameraPos - wSamplePos);

        vec3 scattering = calculateScattering(wSamplePos, wLightToSample, wSampleToCamera);

        //transmittance x scattering x scatteringCoeff x differential of integral
        result += cameraSampleTransmittance*scattering*scatteringCoeff*differential;
        wSamplePos += wStep;
    }
    colorFrag = vec4(result, 1.0);
}

