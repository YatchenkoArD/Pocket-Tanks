// Fragment Shader (fragment_shader.glsl)
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D tankTexture;

void main() {
    FragColor = texture(tankTexture, TexCoord);
}