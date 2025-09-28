#version 300 es

precision mediump float;

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec4 uColor;

void main() {
    vec4 texColor = texture(uTexture, TexCoord);
    FragColor = vec4(uColor.rgb, texColor.r * uColor.a);
}