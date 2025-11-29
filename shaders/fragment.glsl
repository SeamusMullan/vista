#version 330 core

in vec2 TexCoord;
in vec2 FragPos;
in vec2 WindowPos;
in vec3 WorldPos3D;
in float Depth;

out vec4 FragColor;

uniform sampler2D texture1;
uniform sampler2D backgroundTexture;
uniform float selected;
uniform float time;
uniform vec2 windowSize;
uniform vec2 thumbnailPos;
uniform vec2 thumbnailSize;
uniform float cornerRadius;
uniform float isBackground;
uniform vec3 avgColor;
uniform float rotationY;

// ====================
// WINDOWS AERO FROSTED GLASS SHADER
// ====================

// Hash function for noise/grain
float hash(vec2 p) {
    p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
    return fract(sin(p.x + p.y) * 43758.5453);
}

// High-quality multi-pass Gaussian blur for frosted glass
vec4 frostBlur(sampler2D tex, vec2 uv, float radius) {
    vec2 texel = 1.0 / windowSize;
    vec4 result = vec4(0.0);
    float totalWeight = 0.0;

    // Larger kernel for more pronounced blur
    const int samples = 12;
    const float sigma = 4.5;

    for (int x = -samples; x <= samples; x++) {
        for (int y = -samples; y <= samples; y++) {
            vec2 offset = vec2(float(x), float(y)) * texel * radius;
            vec2 sampleUV = clamp(uv + offset, 0.0, 1.0);

            // Gaussian weight
            float dist = length(vec2(x, y));
            float weight = exp(-(dist * dist) / (2.0 * sigma * sigma));

            result += texture(tex, sampleUV) * weight;
            totalWeight += weight;
        }
    }

    return result / totalWeight;
}

// Lighter, faster blur for subtle effects
vec4 softBlur(sampler2D tex, vec2 uv, float radius) {
    vec2 texel = radius / windowSize;
    vec4 result = vec4(0.0);

    const float weights[5] = float[5](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

    result += texture(tex, uv) * weights[0];

    for (int i = 1; i < 5; i++) {
        vec2 off = texel * float(i);
        result += texture(tex, uv + vec2(off.x, 0.0)) * weights[i];
        result += texture(tex, uv - vec2(off.x, 0.0)) * weights[i];
        result += texture(tex, uv + vec2(0.0, off.y)) * weights[i];
        result += texture(tex, uv - vec2(0.0, off.y)) * weights[i];
    }

    return result;
}

// Rounded rectangle distance field
float roundedBoxSDF(vec2 pos, vec2 center, vec2 size, float radius) {
    vec2 d = abs(pos - center) - size * 0.5 + radius;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0) - radius;
}

// Enhanced specular highlights for glass reflection
vec3 glassSpecular(vec2 uv, vec2 center) {
    vec2 toCenter = normalize(center - uv);
    float fresnel = pow(1.0 - abs(dot(toCenter, vec2(0.0, 1.0))), 3.0);

    // Animated light sweep
    float sweep = sin(time * 0.3 + uv.x * 2.0) * 0.5 + 0.5;

    vec3 highlight = vec3(0.9, 0.95, 1.0) * fresnel * 0.15;
    highlight += vec3(0.7, 0.8, 1.0) * sweep * 0.08;

    return highlight;
}

void main() {
    // ======================
    // BACKGROUND RENDERING (Frosted Glass)
    // ======================
    if (isBackground > 0.5) {
        // Multi-layer blur for authentic frosted glass
        vec4 blur1 = frostBlur(texture1, TexCoord, 1.2);
        vec4 blur2 = softBlur(texture1, TexCoord, 8.0);
        vec4 blurredBG = mix(blur1, blur2, 0.6);

        // Windows Aero blue-tinted glass
        vec3 aeroTint = vec3(0.85, 0.92, 0.98);
        vec3 tintedGlass = blurredBG.rgb * aeroTint;

        // Add subtle color shift based on position
        vec2 ndc = (WindowPos - windowSize * 0.5) / (windowSize * 0.5);
        float gradientShift = length(ndc) * 0.15;
        tintedGlass = mix(tintedGlass, tintedGlass * vec3(0.9, 0.95, 1.0), gradientShift);

        // Rounded window corners
        vec2 windowCenter = windowSize * 0.5;
        float windowDist = roundedBoxSDF(WindowPos, windowCenter, windowSize, cornerRadius);
        float windowAlpha = 1.0 - smoothstep(-2.0, 2.0, windowDist);

        // Glass specular highlights
        vec3 specular = glassSpecular(WindowPos / windowSize, vec2(0.5, 0.3));

        // Vignette for depth
        float radial = length(ndc);
        float vignette = 1.0 - smoothstep(0.6, 1.2, radial);
        vec3 vignetted = tintedGlass * (0.85 + vignette * 0.15);

        // Film grain for texture
        float grain = (hash(WindowPos + time * 100.0) - 0.5) * 0.025;

        // Combine all effects
        vec3 finalColor = vignetted + specular + grain;

        // Aero-style transparency (slightly opaque frosted glass)
        float glassAlpha = 0.75 * windowAlpha;

        FragColor = vec4(clamp(finalColor, 0.0, 1.0), glassAlpha);
        return;
    }

    // ======================
    // THUMBNAIL RENDERING (3D Gallery Items)
    // ======================

    vec4 texColor = texture(texture1, TexCoord);

    // Calculate rounded corners for thumbnail
    vec2 thumbnailCenter = thumbnailPos + thumbnailSize * 0.5;
    float thumbDist = roundedBoxSDF(FragPos, thumbnailCenter, thumbnailSize, cornerRadius);

    // Smooth anti-aliased edges
    float edgeSoftness = fwidth(thumbDist) * 1.5;
    float thumbAlpha = 1.0 - smoothstep(-edgeSoftness, edgeSoftness, thumbDist);

    // Apply rounded corners to image
    texColor.a *= thumbAlpha;

    // ======================
    // GLOWING OUTLINE EFFECT
    // ======================

    vec3 glowColor = avgColor;
    // Boost saturation for vibrant glow
    float luminance = dot(glowColor, vec3(0.299, 0.587, 0.114));
    glowColor = mix(vec3(luminance), glowColor, 1.8);
    glowColor = clamp(glowColor, 0.0, 1.0);

    // Glow parameters
    float glowRadius = mix(35.0, 55.0, selected);
    float glowIntensity = mix(0.6, 1.0, selected);

    // Pulsing animation
    float pulse = 0.7 + 0.3 * sin(time * 2.0 + dot(glowColor, vec3(1.0)));

    // Distance-based glow falloff
    float distToEdge = max(thumbDist, 0.0);
    float glow = exp(-pow(distToEdge / glowRadius, 1.3)) * glowIntensity * pulse;

    // Outer glow ring (more pronounced)
    if (thumbDist > 0.0 && thumbDist < glowRadius) {
        vec3 outerGlow = glowColor * glow * 1.2;
        texColor.rgb += outerGlow;
        texColor.a = max(texColor.a, glow * 0.7);
    }

    // Inner glow (selected items get extra shimmer)
    if (thumbDist <= 0.0) {
        float innerGlow = 0.12 * selected + 0.05;
        float shimmer = sin(time * 3.0 + FragPos.x * 0.1) * 0.03;
        texColor.rgb += glowColor * (innerGlow + shimmer);
    }

    // ======================
    // 3D DEPTH & LIGHTING
    // ======================

    // Subtle shading based on rotation angle
    float rotationShade = cos(rotationY) * 0.15 + 0.85;
    texColor.rgb *= rotationShade;

    // Depth-based brightness (items further back are slightly darker)
    float depthFade = 1.0 - (abs(Depth) * 0.08);
    texColor.rgb *= depthFade;

    // Edge lighting (rim light effect)
    vec2 edgeNormal = normalize(FragPos - thumbnailCenter);
    float rimLight = pow(1.0 - thumbAlpha, 3.0) * 0.3;
    texColor.rgb += glowColor * rimLight;

    // ======================
    // REFLECTION/SPECULAR
    // ======================

    // Add subtle glass-like reflection on the image surface
    vec2 reflectUV = TexCoord;
    float reflectiveness = 0.08;
    vec3 reflection = vec3(0.9, 0.95, 1.0) * reflectiveness;

    // Animated light sweep across selected item
    if (selected > 0.5) {
        float sweepPos = fract(time * 0.4);
        float sweep = exp(-pow((TexCoord.x - sweepPos) * 3.0, 2.0));
        texColor.rgb += vec3(1.0, 1.0, 1.0) * sweep * 0.25;
    }

    texColor.rgb += reflection * (0.5 + 0.5 * sin(time * 0.5));

    FragColor = texColor;
}
