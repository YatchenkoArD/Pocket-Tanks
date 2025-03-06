#version 330 core
uniform vec2 tankPosition;
uniform float tankAngle;

void main() {
    mat2 rotation = mat2(
        cos(tankAngle), -sin(tankAngle),
        sin(tankAngle),  cos(tankAngle)
    );
    vec2 rotatedPos = rotation * aPos;
    gl_Position = vec4(rotatedPos + tankPosition, 0.0, 1.0);
    TexCoord = aTexCoord;
}