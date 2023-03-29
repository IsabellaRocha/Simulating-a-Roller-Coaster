#version 150

in vec3 position;
in vec2 texCoord;
out vec2 tc;

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;

void main() {
    // compute the transformed and projected vertex position (into gl_Position)
    // compute the textureCoors (into tc)
    gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0f);
    // pass-through the texture coordinate
    tc = texCoord;

}