#version 150

in vec2 tc;
out vec4 c;
uniform sampler2D textureImage;

void main() {
    // compute the final fragment color,
    c = texture(textureImage, tc);
}