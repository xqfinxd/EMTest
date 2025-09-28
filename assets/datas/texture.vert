#version 300 es

precision mediump float;

in vec4 aVertex;

out vec2 TexCoord;

uniform mat4 mvp;

void main() {
    gl_Position = mvp * vec4(aVertex.xy, 0.0, 1.0);
    TexCoord = aVertex.zw;
}