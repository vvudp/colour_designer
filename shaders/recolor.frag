#version 330 core

in vec2 vTexCoord;
out vec4 fragColor;

uniform sampler2D uOriginal;
uniform sampler2D uUpperMask;
uniform sampler2D uLowerMask;

uniform vec3 uUpperColor;
uniform vec3 uLowerColor;
uniform float uUpperReference;
uniform float uLowerReference;
uniform float uMatte;
uniform float uHighlight;
uniform float uShadow;
uniform float uBrightness;
uniform bool uShowOriginal;

float srgbChannelToLinear(float c)
{
    return c <= 0.04045
        ? c / 12.92
        : pow((c + 0.055) / 1.055, 2.4);
}

vec3 srgbToLinear(vec3 c)
{
    return vec3(
        srgbChannelToLinear(c.r),
        srgbChannelToLinear(c.g),
        srgbChannelToLinear(c.b));
}

float linearChannelToSrgb(float c)
{
    c = max(c, 0.0);
    return c <= 0.0031308
        ? c * 12.92
        : 1.055 * pow(c, 1.0 / 2.4) - 0.055;
}

vec3 linearToSrgb(vec3 c)
{
    return vec3(
        linearChannelToSrgb(c.r),
        linearChannelToSrgb(c.g),
        linearChannelToSrgb(c.b));
}

float smoothRamp(float edge0, float edge1, float x)
{
    float t = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

vec3 recolorRegion(
    vec3 sourceLinear,
    float sourceLuminance,
    vec3 targetSrgb,
    float referenceLuminance,
    float mask)
{
    if (mask <= 0.0001) {
        return sourceLinear;
    }

    vec3 targetLinear = srgbToLinear(targetSrgb) * uBrightness;
    float relativeLight = clamp(
        sourceLuminance / max(referenceLuminance, 0.0001),
        0.055,
        2.4
    );

    float exponent = 0.82 + 0.20 * clamp(uShadow, 0.0, 2.0);
    float shade = pow(relativeLight, exponent);
    shade = min(shade, 1.85 - clamp(uMatte, 0.0, 1.0) * 0.35);

    vec3 painted = targetLinear * shade;

    float highlightMask = smoothRamp(1.02, 1.72, relativeLight);
    float neutralStrength = highlightMask
        * clamp(uHighlight, 0.0, 1.0)
        * (1.0 - clamp(uMatte, 0.0, 1.0) * 0.45);
    vec3 neutralHighlight = vec3(sourceLuminance * neutralStrength);
    painted = max(painted, neutralHighlight);

    return mix(sourceLinear, clamp(painted, 0.0, 1.0), mask);
}

void main()
{
    vec4 source = texture(uOriginal, vTexCoord);
    if (uShowOriginal) {
        fragColor = source;
        return;
    }

    float upperMask = texture(uUpperMask, vTexCoord).r;
    float lowerMask = texture(uLowerMask, vTexCoord).r;

    vec3 sourceLinear = srgbToLinear(source.rgb);
    float luminance = dot(sourceLinear, vec3(0.2126, 0.7152, 0.0722));

    vec3 result = sourceLinear;
    result = recolorRegion(result, luminance, uUpperColor, uUpperReference, upperMask);
    result = recolorRegion(result, luminance, uLowerColor, uLowerReference, lowerMask);

    fragColor = vec4(linearToSrgb(result), source.a);
}
