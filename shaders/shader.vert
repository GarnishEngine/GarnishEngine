#version 330 core

in vec3 aPos;

in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 mvp;

void main() {
   gl_Position = mvp * vec4(aPos, 1.0);
   TexCoord=aTexCoord;
}
