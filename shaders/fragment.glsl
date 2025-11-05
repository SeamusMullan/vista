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

// Sample texture with blur
vec4 sampleBlurred(sampler2D tex, vec2 uv, float blur) {
    if (blur < 0.001) {
        return texture(tex, uv);
    }
    
    vec4 color = vec4(0.0);
    float total = 0.0;
    
    // Simple box blur
    int samples = 9;
    float offset = blur * 0.002;
    
    for (int x = -samples/2; x <= samples/2; x++) {
        for (int y = -samples/2; y <= samples/2; y++) {
            vec2 sampleUV = uv + vec2(float(x), float(y)) * offset;
            if (sampleUV.x >= 0.0 && sampleUV.x <= 1.0 && sampleUV.y >= 0.0 && sampleUV.y <= 1.0) {
                color += texture(tex, sampleUV);
                total += 1.0;
            }
        }
    }
    
    return color / total;
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
        // Apply blur to background
        vec4 blurredColor = sampleBlurred(texture1, TexCoord, blurStrength);
        
        // Apply rounded corners to window
        vec2 windowCenter = windowSize * 0.5;
        float dist = roundedRectDistance(WindowPos, windowCenter, windowSize, cornerRadius);
        
        // Smooth edge for anti-aliasing
        float alpha = 1.0 - smoothstep(-1.0, 1.0, dist);
        
        // Apply transparency to entire window background
        FragColor = vec4(blurredColor.rgb, alpha * 0.95); // 95% transparent
        return;
    }
    
    // For thumbnail rendering
    vec2 thumbnailCenter = thumbnailPos + thumbnailSize * 0.5;
    float thumbnailDist = roundedRectDistance(FragPos, thumbnailCenter, thumbnailSize, cornerRadius * 0.5);
    
    // Apply rounded corners to thumbnail
    float thumbnailAlpha = 1.0 - smoothstep(-1.0, 1.0, thumbnailDist);
    texColor.a *= thumbnailAlpha;
    
    // Glow effect for selected thumbnail
    if (selected > 0.5) {
        // Use provided average color or extract it
        vec3 glowColor = avgColor;
        if (length(glowColor) < 0.1) {
            // Fallback: extract dominant color from thumbnail
            glowColor = extractDominantColor(texture1, vec2(0.5, 0.5));
        }
        
        // Enhance saturation for glow
        float luminance = dot(glowColor, vec3(0.299, 0.587, 0.114));
        glowColor = mix(vec3(luminance), glowColor, 1.5); // Increase saturation
        glowColor = clamp(glowColor, 0.0, 1.0);
        
        // Pulsating glow intensity
        float glowPulse = 0.6 + 0.4 * sin(time * 2.0);
        
        // Distance-based glow
        float glowRadius = 40.0; // Pixels
        float glowDist = max(0.0, thumbnailDist);
        float glowIntensity = exp(-glowDist / glowRadius) * glowPulse;
        
        // Add glow around thumbnail
        if (thumbnailDist > 0.0 && thumbnailDist < glowRadius) {
            vec3 glow = glowColor * glowIntensity * 0.8;
            texColor.rgb += glow;
            texColor.a = max(texColor.a, glowIntensity * 0.6);
        }
        
        // Inner highlight on thumbnail itself
        if (thumbnailDist <= 0.0) {
            float innerGlow = 0.15 + 0.1 * sin(time * 3.0);
            texColor.rgb += glowColor * innerGlow;
        }
    }
    
    FragColor = texColor;
}
