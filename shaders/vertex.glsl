#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;
out vec2 FragPos;
out vec2 WindowPos;
out vec3 WorldPos3D;
out float Depth;

uniform mat4 projection;
uniform mat4 model;
uniform vec2 windowSize;
uniform float rotationY;      // Y-axis rotation for 3D carousel effect
uniform float tiltX;          // X-axis tilt (lean forward)
uniform float depth3D;        // Z-depth for perspective
uniform float is3D;           // Toggle 3D transformation

void main() {
    vec4 pos = vec4(aPos, 0.0, 1.0);

    // Apply 3D transformations for thumbnail carousel effect
    if (is3D > 0.5) {
        // Center the quad for rotation
        vec2 centered = aPos;

        // Build 3D rotation matrices
        float cosY = cos(rotationY);
        float sinY = sin(rotationY);
        float cosX = cos(tiltX);
        float sinX = sin(tiltX);

        // Apply Y-axis rotation (carousel spin)
        mat4 rotY = mat4(
            cosY,  0.0, sinY, 0.0,
            0.0,   1.0, 0.0,  0.0,
            -sinY, 0.0, cosY, 0.0,
            0.0,   0.0, 0.0,  1.0
        );

        // Apply X-axis rotation (tilt forward)
        mat4 rotX = mat4(
            1.0, 0.0,   0.0,    0.0,
            0.0, cosX, -sinX,   0.0,
            0.0, sinX,  cosX,   0.0,
            0.0, 0.0,   0.0,    1.0
        );

        // Apply depth translation
        mat4 translate = mat4(
            1.0, 0.0, 0.0, 0.0,
            0.0, 1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            0.0, 0.0, depth3D, 1.0
        );

        // Combine transformations: translation -> rotation
        pos = model * translate * rotX * rotY * vec4(centered, 0.0, 1.0);
        WorldPos3D = pos.xyz;
        Depth = pos.z;
    } else {
        pos = model * pos;
        WorldPos3D = vec3(pos.xy, 0.0);
        Depth = 0.0;
    }

    gl_Position = projection * pos;
    TexCoord = aTexCoord;

    // Pass fragment position in screen space for rounded corners
    FragPos = pos.xy;

    // Window position for effects
    WindowPos = (gl_Position.xy * 0.5 + 0.5) * windowSize;
}
