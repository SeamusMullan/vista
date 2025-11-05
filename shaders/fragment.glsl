#version 330 core

in vec2 TexCoord;
in vec2 FragPos;
in vec2 WindowPos;

out vec4 FragColor;

uniform sampler2D texture1;
uniform sampler2D backgroundTexture;
uniform float selected;
uniform float time;
uniform vec2 windowSize;
uniform vec2 thumbnailPos;  // Position of thumbnail in screen space
uniform vec2 thumbnailSize; // Size of thumbnail
uniform float cornerRadius; // Radius for rounded corners
uniform float isBackground;  // 1.0 if rendering background, 0.0 for thumbnails
uniform vec3 avgColor;      // Average color from thumbnail for glow
uniform float blurStrength; // Strength of background blur

// Pseudo random hash for subtle film grain
float hash(vec2 p) {
    p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
    return fract(sin(p.x + p.y) * 43758.5453);
}

// Proper separable Gaussian blur for glass-like softness
vec4 sampleGlassBlur(sampler2D tex, vec2 uv, float radius) {
    if (radius <= 0.001) {
        return texture(tex, uv);
    }
    
    vec2 texel = radius / windowSize;
    vec4 accum = vec4(0.0);
    float weightAccum = 0.0;
    
    // Gaussian weights for 7-tap kernel (sigma ≈ 2.0)
    // Pre-calculated for performance
    const float weights[7] = float[7](
        0.0702, 0.1311, 0.1907, 0.2161, 0.1907, 0.1311, 0.0702
    );
    
    // Two-pass separable Gaussian blur
    // Horizontal pass
    vec4 horizontalBlur = vec4(0.0);
    for (int i = 0; i < 7; ++i) {
        float offset = float(i - 3);
        vec2 sampleUV = clamp(uv + vec2(texel.x * offset, 0.0), 0.0, 1.0);
        horizontalBlur += texture(tex, sampleUV) * weights[i];
    }
    
    // Vertical pass (simulated by sampling diagonal and vertical)
    for (int i = 0; i < 7; ++i) {
        float offset = float(i - 3);
        vec2 sampleUV = clamp(uv + vec2(0.0, texel.y * offset), 0.0, 1.0);
        accum += texture(tex, sampleUV) * weights[i];
    }
    
    // Blend horizontal and vertical for approximate 2D Gaussian
    return (horizontalBlur + accum) * 0.5;
}

// Extract dominant color from texture (simplified approach)
vec3 extractDominantColor(sampler2D tex, vec2 uv) {
    // Sample at multiple points and average
    vec3 color = vec3(0.0);
    int samples = 16;
    
    for (int i = 0; i < samples; i++) {
        float angle = float(i) * 6.28318 / float(samples);
        vec2 offset = vec2(cos(angle), sin(angle)) * 0.3;
        vec2 sampleUV = clamp(uv + offset, 0.0, 1.0);
        color += texture(tex, sampleUV).rgb;
    }
    
    // Add center sample with more weight
    color += texture(tex, uv).rgb * 4.0;
    
    return color / float(samples + 4);
}

// Distance to rounded rectangle
float roundedRectDistance(vec2 pos, vec2 center, vec2 size, float radius) {
    vec2 d = abs(pos - center) - size * 0.5 + radius;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0) - radius;
}

void main() {
    vec4 texColor = texture(texture1, TexCoord);
    
    // For background rendering
    if (isBackground > 0.5) {
        // Apply multi-axis blur to background for frosted-glass look
        vec4 blurredColor = sampleGlassBlur(texture1, TexCoord, 15.5);

        // Apply rounded corners to window
        vec2 windowCenter = windowSize * 0.5;
        float dist = roundedRectDistance(WindowPos, windowCenter, windowSize, cornerRadius);
        
        // Calculate the magnitude of the distance field's change across one pixel.
        // Multiply by a small factor (like 1.5) to widen the blur slightly for a softer edge.
        float aa_width = fwidth(dist) * 1.5; 
        
        // Use the derivative (aa_width) to define the smoothstep range.
        // This ensures the transition from opaque to transparent is exactly one pixel wide.
        float alpha = 1.0 - smoothstep(-aa_width, aa_width, dist);

        // Subtle tint keeps the glass cohesive even with busy wallpapers
        vec3 glassTint = vec3(0.12, 0.14, 0.18);
        vec3 tinted = mix(blurredColor.rgb, glassTint + blurredColor.rgb * 0.75, 0.35);

        // Soft vignette and highlight for depth
        vec2 ndc = (WindowPos - windowCenter) / windowCenter;
        float radial = clamp(dot(ndc, ndc), 0.0, 2.0);
        float vignette = 1.0 - smoothstep(0.72, 0.98, radial);
        float highlight = pow(clamp(1.0 - radial, 0.0, 1.0), 3.0) * 0.22;
        vec3 lighting = mix(tinted * 0.9, tinted, vignette) + vec3(0.32, 0.34, 0.42) * highlight;

        // Time varying grain breaks banding and sells the material
        float grain = (hash(WindowPos * 0.75 + time * 0.5) - 0.5) * 0.02;
        lighting += grain;

        FragColor = vec4(clamp(lighting, 0.0, 1.0), alpha * 0.82);
        return;
    }
    
    // For thumbnail rendering
    vec2 thumbnailCenter = thumbnailPos + thumbnailSize * 0.5;
    float thumbnailDist = roundedRectDistance(FragPos, thumbnailCenter, thumbnailSize, cornerRadius * 0.5);
    
    // Apply rounded corners to thumbnail
    float thumbnailAlpha = 1.0 - smoothstep(-1.0, 1.0, thumbnailDist);
    texColor.a *= thumbnailAlpha;
    
    // Colored blur halo around every thumbnail, intensified when selected
    vec3 glowColor = avgColor;
    if (length(glowColor) < 0.1) {
        glowColor = extractDominantColor(texture1, vec2(0.5, 0.5));
    }

    float glowRadius = mix(28.0, 44.0, selected);
    float haloDist = max(thumbnailDist, 0.0);
    float haloIntensity = exp(-pow(haloDist / max(glowRadius, 0.001), 1.25));
    float pulse = 0.55 + 0.35 * sin(time * 2.4 + dot(glowColor, vec3(2.1, 1.3, 3.7)));
    float haloBoost = mix(0.45, 0.95, selected) * pulse;

    vec3 saturatedGlow = clamp(mix(vec3(dot(glowColor, vec3(0.299, 0.587, 0.114))), glowColor, 1.7), 0.0, 1.0);
    vec3 halo = saturatedGlow * haloIntensity * haloBoost;

    if (thumbnailDist > -glowRadius) {
        texColor.rgb += halo;
        texColor.a = max(texColor.a, haloIntensity * 0.55 * mix(0.6, 1.0, selected));
    }

    // Inner lift keeps the artwork readable while sitting on the glass
    if (thumbnailDist <= 0.0) {
        float innerGlow = 0.08 + 0.06 * selected + 0.04 * sin(time * 3.2);
        texColor.rgb = clamp(texColor.rgb + saturatedGlow * innerGlow, 0.0, 1.0);
    }
    
    FragColor = texColor;
}
