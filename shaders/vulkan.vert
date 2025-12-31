#version 450

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
    vec3 viewPos;
} camera;

layout(set = 0, binding = 1) readonly buffer ModelMatrices {
    mat4 models[];
};

layout(set = 0, binding = 3) uniform LightingUBO {
    vec3 lightPos;
    vec3 lightColor;
} lighting;

layout(push_constant) uniform PC {
    uint texIndex;
    uint modelIndex;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;

void main() {
    mat4 model = models[pc.modelIndex];
    vec4 worldPos = model * vec4(inPosition, 1.0);
    fragPos = worldPos.xyz;
    fragNormal = mat3(transpose(inverse(model))) * inNormal;
    fragTexCoord = inTexCoord;
    gl_Position = camera.proj * camera.view * worldPos;
}
