#version 330

in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec2 texSize;
uniform float amount;

out vec4 finalColor;

void main() {
    vec4 texel = texture(texture0, fragTexCoord);
    float r = texel.r;
    float g = texel.g;
    float b = texel.b;
    float sr = 0.393 * r + 0.769 * g + 0.189 * b;
    float sg = 0.349 * r + 0.686 * g + 0.168 * b;
    float sb = 0.272 * r + 0.534 * g + 0.131 * b;
    vec3 sepia = vec3(sr, sg, sb);
    vec3 mixed = mix(texel.rgb, sepia, amount);
    finalColor = vec4(mixed, texel.a) * colDiffuse * fragColor;
}