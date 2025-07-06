#version 450
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_samplerless_texture_functions : enable

layout(set = 0, binding = 0) uniform UniformBufferObject { mat4 mvp; } ubo;
layout(set = 0, binding = 1) uniform sampler texSampler;
layout(set = 0, binding = 2) uniform texture2D textures[];


layout(push_constant) uniform PC { uint texIndex; } pc;

layout(location = 0) in  vec3 fragColor;     
layout(location = 1) in  vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;      

void main()
{
    vec4 albedo = texture(
                        sampler2D(textures[nonuniformEXT(pc.texIndex)],
                        texSampler), 
                    fragTexCoord);
    outColor    = albedo * vec4(fragColor, 1.0);
}
