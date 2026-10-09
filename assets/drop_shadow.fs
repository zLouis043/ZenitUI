#version 330

in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec4 shadowColor;

out vec4 finalColor;

void main() {
    vec4 texel = texture(texture0, fragTexCoord);
    // Prendi la silhouette: alpha del texel * alpha dello shadow color,
    // RGB = shadow color.
    finalColor = vec4(shadowColor.rgb, texel.a * shadowColor.a) * colDiffuse * fragColor;
}