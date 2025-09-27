#version 300 es

precision mediump float;

in vec2 aPos;

out vec2 TexCoord;

uniform mat4 mvp;

void main() {
    gl_Position = mvp * vec4(aPos, 0.0, 1.0);
    TexCoord = aPos + 0.5f;
}