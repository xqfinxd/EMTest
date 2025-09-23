#version 300 es

precision mediump float;

in vec2 aPos;

out vec2 TexCoord;

uniform mat4 mvp;
uniform vec2 offset;
uniform vec2 size;

void main() {
    gl_Position = mvp * vec4(aPos, 0.0, 1.0);
	vec2 texPos = aPos + 0.5f;
	texPos.x = texPos.x * size.x;
	texPos.y = texPos.y * size.y;
    TexCoord = texPos + offset;
}