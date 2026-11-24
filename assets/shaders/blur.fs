#version 330

in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec2 texSize;
uniform vec2 direction;
uniform float radius;

out vec4 finalColor;

void main() {
    vec2 step = direction * radius / texSize;

    const float w0 = 0.2270270270;
    const float w1 = 0.1945945946;
    const float w2 = 0.1216216216;
    const float w3 = 0.0540540541;
    const float w4 = 0.0162162162;

    vec4 sum = texture(texture0, fragTexCoord) * w0;
    sum += texture(texture0, fragTexCoord + step * 1.0) * w1;
    sum += texture(texture0, fragTexCoord - step * 1.0) * w1;
    sum += texture(texture0, fragTexCoord + step * 2.0) * w2;
    sum += texture(texture0, fragTexCoord - step * 2.0) * w2;
    sum += texture(texture0, fragTexCoord + step * 3.0) * w3;
    sum += texture(texture0, fragTexCoord - step * 3.0) * w3;
    sum += texture(texture0, fragTexCoord + step * 4.0) * w4;
    sum += texture(texture0, fragTexCoord - step * 4.0) * w4;

    finalColor = sum * colDiffuse * fragColor;
}