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
// WINDOWS AERO FROSTED GLASS SHADER (FIXED)
// ====================

// Hash function for noise/grain
float hash(vec2 p) {
    p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
    return fract(sin(p.x + p.y) * 43758.5453);
}

// Optimized blur - uses fewer samples but still looks good
vec4 frostBlur(sampler2D tex, vec2 uv, float radius) {
    vec2 texel = 1.0 / windowSize;
    vec4 result = vec4(0.0);
    float totalWeight = 0.0;
    
    // Reduced kernel for better performance
    const int samples = 8;
    const float sigma = 3.5;
    
    for (int x = -samples; x <= samples; x++) {
        for (int y = -samples; y <= samples; y++) {
            vec2 offset = vec2(float(x), float(y)) * texel * radius;
            vec2 sampleUV = clamp(uv + offset, 0.001, 0.999);
            
            float dist = length(vec2(x, y));
            float weight = exp(-(dist * dist) / (2.0 * sigma * sigma));
            
            result += texture(tex, sampleUV) * weight;
            totalWeight += weight;
        }
    }
    
    return result / totalWeight;
}

// Rounded rectangle distance field - FIXED for proper corner handling
float roundedBoxSDF(vec2 pos, vec2 center, vec2 halfSize, float radius) {
    vec2 d = abs(pos - center) - halfSize + radius;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0) - radius;
}

// Glass specular highlights
vec3 glassSpecular(vec2 uv, vec2 center) {
    vec2 toCenter = normalize(center - uv + 0.001);
    float fresnel = pow(1.0 - abs(dot(toCenter, vec2(0.0, 1.0))), 3.0);
    
    // Animated light sweep
    float sweep = sin(time * 0.3 + uv.x * 2.0) * 0.5 + 0.5;
    
    vec3 highlight = vec3(0.9, 0.95, 1.0) * fresnel * 0.12;
    highlight += vec3(0.7, 0.8, 1.0) * sweep * 0.06;
    
    return highlight;
}

void main() {
    // ======================
    // BACKGROUND RENDERING (Transparent with subtle tint)
    // ======================
    if (isBackground > 0.5) {
        // TRANSPARENT BACKGROUND - let the desktop show through!
        // We just add a subtle tinted overlay with rounded corners
        
        vec2 ndc = (WindowPos - windowSize * 0.5) / (windowSize * 0.5);
        
        // Rounded window corners
        vec2 windowCenter = windowSize * 0.5;
        vec2 windowHalfSize = windowSize * 0.5;
        float windowDist = roundedBoxSDF(WindowPos, windowCenter, windowHalfSize, cornerRadius);
        
        // Smooth edge with anti-aliasing
        float edgeWidth = 1.5;
        float windowAlpha = 1.0 - smoothstep(-edgeWidth, edgeWidth, windowDist);
        
        // Subtle dark tint so thumbnails are visible against any background
        // Adjust these values to taste:
        //   - vec3 controls the tint color (darker = more contrast)
        //   - the alpha (0.3) controls how transparent (lower = more see-through)
        vec3 tintColor = vec3(0.08, 0.08, 0.12); // Dark blue-ish tint
        float tintAlpha = 0.4; // 40% opacity - adjust this!
        
        // Optional: slight gradient for depth
        float gradient = 1.0 - length(ndc) * 0.15;
        tintColor *= gradient;
        
        // Optional: subtle vignette darkening at edges
        float vignette = smoothstep(0.5, 1.2, length(ndc));
        tintAlpha += vignette * 0.15;
        
        // Film grain for texture (very subtle)
        float grain = (hash(WindowPos + time * 100.0) - 0.5) * 0.02;
        tintColor += grain;
        
        FragColor = vec4(clamp(tintColor, 0.0, 1.0), tintAlpha * windowAlpha);
        return;
    }
    
    // ======================
    // THUMBNAIL RENDERING
    // ======================
    
    vec4 texColor = texture(texture1, TexCoord);
    
    // Calculate rounded corners for thumbnail
    vec2 thumbnailCenter = thumbnailPos + thumbnailSize * 0.5;
    vec2 thumbnailHalfSize = thumbnailSize * 0.5;
    float thumbDist = roundedBoxSDF(FragPos, thumbnailCenter, thumbnailHalfSize, cornerRadius);
    
    // FIXED: Better anti-aliased edges
    float edgeSoftness = 1.5; // Fixed pixel width for smooth edges
    float thumbAlpha = 1.0 - smoothstep(-edgeSoftness, edgeSoftness, thumbDist);
    
    // ======================
    // GLOWING OUTLINE EFFECT (FIXED)
    // ======================
    
    vec3 glowColor = avgColor;
    // Boost saturation for vibrant glow
    float luminance = dot(glowColor, vec3(0.299, 0.587, 0.114));
    glowColor = mix(vec3(luminance), glowColor, 1.6);
    glowColor = clamp(glowColor * 1.2, 0.0, 1.0); // Brighten
    
    // Glow parameters - stronger for selected
    float glowRadius = mix(20.0, 40.0, selected);
    float glowIntensity = mix(0.4, 0.9, selected);
    
    // Pulsing animation (only when time is updating!)
    float pulse = 0.75 + 0.25 * sin(time * 2.5 + luminance * 6.28);
    
    // FIXED: Glow that extends outside the thumbnail
    float distToEdge = max(thumbDist, 0.0);
    float glowFalloff = exp(-distToEdge / glowRadius * 2.5);
    float glow = glowFalloff * glowIntensity * pulse;
    
    // Start with the texture color, masked by rounded corners
    vec4 result = texColor;
    result.a *= thumbAlpha;
    
    // Add outer glow (outside the thumbnail bounds)
    if (thumbDist > 0.0) {
        // Pure glow outside the image
        vec3 outerGlow = glowColor * glow;
        float glowAlpha = glow * 0.8;
        
        // Blend glow with existing (for the edge transition)
        result.rgb = mix(result.rgb, outerGlow, 1.0 - thumbAlpha);
        result.a = max(result.a, glowAlpha);
    }
    
    // Add inner glow/highlight on selected items
    if (thumbDist <= 0.0) {
        float innerGlow = selected * 0.15;
        float shimmer = sin(time * 3.0 + FragPos.x * 0.05 + FragPos.y * 0.03) * 0.04 * selected;
        result.rgb += glowColor * (innerGlow + shimmer);
        
        // Subtle edge highlight
        float edgeHighlight = smoothstep(-10.0, 0.0, thumbDist) * (1.0 - smoothstep(-2.0, 0.0, thumbDist));
        result.rgb += glowColor * edgeHighlight * 0.3 * selected;
    }
    
    // ======================
    // 3D DEPTH & LIGHTING
    // ======================
    
    // Subtle shading based on rotation
    float rotationShade = cos(rotationY) * 0.12 + 0.88;
    result.rgb *= rotationShade;
    
    // Depth-based brightness
    float depthFade = 1.0 - (abs(Depth) * 0.06);
    result.rgb *= depthFade;
    
    // ======================
    // REFLECTION/SPECULAR (selected items)
    // ======================
    
    if (selected > 0.5 && thumbDist <= 0.0) {
        // Animated light sweep
        float sweepPos = fract(time * 0.3);
        float sweepWidth = 0.15;
        float sweep = smoothstep(sweepPos - sweepWidth, sweepPos, TexCoord.x) 
                    * (1.0 - smoothstep(sweepPos, sweepPos + sweepWidth, TexCoord.x));
        result.rgb += vec3(1.0) * sweep * 0.2;
    }
    
    // Subtle glass reflection on all thumbnails
    float reflectiveness = 0.05 + 0.03 * selected;
    vec3 reflection = vec3(0.95, 0.97, 1.0) * reflectiveness;
    result.rgb += reflection * (0.6 + 0.4 * sin(time * 0.4));
    
    FragColor = result;
}