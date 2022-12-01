#version 410 core

const float PI = 3.14159265359;
const int NSAMPLES = 10;
const float E = 0.5772156649;

out vec4 colorFrag;

uniform vec3 wLightPos;
uniform vec3 wCameraPos;

in vec3 wPos;
in vec3 wNormal;

in vec2 interp_UV;
in vec3 interp_UVW;

uniform samplerCube tCube;
// texture repetitions
uniform float repeat;
// texture sampler
uniform sampler2D tex;
// texture sampler for the depth map
uniform samplerCube depthMap;
uniform float alpha; // rugosity - 0 : smooth, 1: rough
uniform float F0; // fresnel reflectance at normal incidence
uniform float Kd; // weight of diffuse reflection
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

//////////////////////////////////////////
// Schlick-GGX method for geometry obstruction (used by GGX model)
float G1(float angle, float alpha)
{
    // in case of Image Based Lighting, the k factor is different:
    // usually it is set as k=(alpha*alpha)/2
    float r = (alpha + 1.0);
    float k = (r*r) / 8.0;

    float num   = angle;
    float denom = angle * (1.0 - k) + k;

    return num / denom;
}

vec3 calculateSurfaceRadiance(vec3 wLightDir, vec3 wViewDir) {
     // we repeat the UVs and we sample the texture
    vec2 repeated_Uv = mod(interp_UV*repeat, 1.0);
    vec4 surfaceColor = texture(tex, repeated_Uv);

    // normalization of the per-fragment normal
    vec3 N = normalize(wNormal);
    // per-fragment light incidence direction
    vec3 L = wLightDir;

    // cosine angle between direction of light and normal
    float NdotL = max(dot(N, L), 0.0);

    // diffusive (Lambert) reflection component
    vec3 lambert = (Kd*surfaceColor.rgb)/PI;

    // we initialize the specular component
    vec3 specular = vec3(0.0);

    // initialization of shadow value
    float shadow = 0.0;

    // if the cosine of the angle between direction of light and normal is positive, then I can calculate the specular component
    if(NdotL > 0.0)
    {
        // the view vector has been calculated in the vertex shader, already negated to have direction from the mesh to the camera
        vec3 V = wViewDir;

        // half vector
        vec3 H = normalize(L + V);

        // we implement the components seen in the slides for a PBR BRDF
        // we calculate the cosines and parameters to be used in the different components
        float NdotH = max(dot(N, H), 0.0);
        float NdotV = max(dot(N, V), 0.0);
        float VdotH = max(dot(V, H), 0.0);
        float alpha_Squared = alpha * alpha;
        float NdotH_Squared = NdotH * NdotH;

        // Geometric factor G2
        float G2 = G1(NdotV, alpha)*G1(NdotL, alpha);

        // Rugosity D
        // GGX Distribution
        float D = alpha_Squared;
        float denom = (NdotH_Squared*(alpha_Squared-1.0)+1.0);
        D /= PI*denom*denom;

        // Fresnel reflectance F (approx Schlick)
        vec3 F = vec3(pow(1.0 - VdotH, 5.0));
        F *= (1.0 - F0);
        F += F0;

        // we put everything together for the specular component
        specular = (F * G2 * D) / (4.0 * NdotV * NdotL);

    }

    float shadowVal = calculateShadow(wPos);
    // the rendering equation is:
    //integral of: BRDF * Li * (cosine angle between N and L)
    // BRDF in our case is: the sum of Lambert and GGX
    // Li is considered as equal to 1: light is white, and we have not applied attenuation. With colored lights, and with attenuation, the code must be modified and the Li factor must be multiplied to finalColor
    //We weight using the shadow value
    // N.B. ) shadow value = 1 -> fragment is in shadow
    //        shadow value = 0 -> fragment is in light
    // Therefore, we use (1-shadow) as weight to apply to the illumination model
    vec3 finalColor = (1.0-shadowVal)*(lambert + specular)*NdotL;

    return finalColor;
}


float random(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

void main() {
    extinctionCoeff = absorptionCoeff + scatteringCoeff;
    // direction of incoming light
    vec3 wFragToLight = normalize(wLightPos - wPos);
    vec3 wFragToCamera = normalize(wCameraPos - wPos);
    vec3 fragRadiance = calculateSurfaceRadiance(wFragToLight, wFragToCamera);

    vec3 wCamToFrag = wPos - wCameraPos;
    float distanceFromCamera = length(wCamToFrag);
    vec3 fragCameraTransmittance = calculateTransmittance(distanceFromCamera);
    
    //1st term
    vec3 transmittedSurfaceRadiance = fragCameraTransmittance*fragRadiance;

    //Ray marching init
    // NSAMPLES + 1 because we want to esclude samples at camera pos and at fragment pos
    vec3 wStep = wCamToFrag / (NSAMPLES+1);
    float differential = length(wStep);

    // Addind randomness to avoid visual artifact (but introduce grain)
    float rand = random(wStep.xy * wStep.z);
    vec3 wSamplePos = wCameraPos + wStep * rand;

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
