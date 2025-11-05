#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;
out vec2 FragPos;
out vec2 WindowPos;

uniform mat4 projection;
uniform mat4 model;
uniform vec2 windowSize;

void main() {
    gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
    
    // Pass fragment position in screen space for rounded corners
    vec4 worldPos = model * vec4(aPos, 0.0, 1.0);
    FragPos = worldPos.xy;
    
    // Window position normalized to [0,1]
    WindowPos = (gl_Position.xy * 0.5 + 0.5) * windowSize;
}
