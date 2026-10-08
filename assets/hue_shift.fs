#version 330

in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;

out vec4 finalColor;

void main() {
    // 1) Riproduci la composizione di default di Raylib:
    //    texture * colDiffuse * fragColor. Per un fill: bianco * 1.0 * colore = colore.
    vec4 base = texture(texture0, fragTexCoord) * colDiffuse * fragColor;

    // 2) Ora agisci sul colore finale. Swap R <-> B.
    //    Il blu #0079F1 = (0, 121, 241) diventa (241, 121, 0) → arancio.
    finalColor = vec4(base.b, base.g, base.r, base.a);
}