#version 330 core

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D texture1;
uniform float selected;
uniform float time;

void main() {
    vec4 texColor = texture(texture1, TexCoord);
    
    if (selected > 0.5) {
        // Highlight selected thumbnail
        float glow = 0.2 + 0.1 * sin(time * 3.0);
        texColor.rgb += vec3(glow);
    }
    
    FragColor = texColor;
}
