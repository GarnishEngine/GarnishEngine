#version 450

layout(set = 0, binding = 0) uniform UBO {
    mat4 mvp;
} ubo;
const uint MAX_TEX = 16;                     
layout(set = 0, binding = 1) uniform sampler2D u_textures[MAX_TEX];
layout(push_constant) uniform PC {           
    uint texIndex;                           
} pc;

layout(location = 0) in  vec3 fragColor;     
layout(location = 1) in  vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;      

void main()
{
    vec4 albedo = texture(u_textures[pc.texIndex], fragTexCoord);
    outColor    = albedo * vec4(fragColor, 1.0);
}
