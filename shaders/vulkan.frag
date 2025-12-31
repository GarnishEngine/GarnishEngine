#version 450
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_samplerless_texture_functions : enable

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
    vec3 viewPos;
} camera;

layout(set = 0, binding = 2) uniform sampler texSampler;

layout(set = 0, binding = 3) uniform LightingUBO {
    vec3 lightPos;
    vec3 lightColor;
} lighting;

layout(set = 0, binding = 4) uniform texture2D textures[];

layout(push_constant) uniform PC {
    uint texIndex;
    uint modelIndex;
    vec3 material_ambient;
    vec3 material_diffuse;
    vec3 material_specular;
    float material_shininess;
} pc;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 texColor = texture(
        sampler2D(textures[nonuniformEXT(pc.texIndex)], texSampler),
        fragTexCoord
    ).rgb;

    vec3 norm = normalize(fragNormal);

    // Ambient
    vec3 ambient = pc.material_ambient * lighting.lightColor * texColor;

    // Diffuse
    vec3 lightDir = normalize(lighting.lightPos - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = pc.material_diffuse * diff * lighting.lightColor * texColor;

    // Specular
    vec3 viewDir = normalize(camera.viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), pc.material_shininess);
    vec3 specular = pc.material_specular * spec * lighting.lightColor;

    vec3 result = ambient + diffuse + specular;
    outColor = vec4(result, 1.0);
}
