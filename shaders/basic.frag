#version 450

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

layout(push_constant) uniform PushConstants {
    vec3 lightDir;
    vec3 lightColor;
    vec3 viewPos;
} pc;

void main() {
    vec3 color = texture(texSampler, fragTexCoord).rgb;
    vec3 normal = normalize(fragNormal);
    
    // Ambient
    vec3 ambient = 0.1 * color;
    
    // Diffuse
    vec3 lightDir = normalize(-pc.lightDir);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * pc.lightColor;
    
    // Specular
    vec3 viewDir = normalize(pc.viewPos - fragPosition);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64);
    vec3 specular = spec * pc.lightColor;
    
    vec3 result = (ambient + diffuse + specular) * color;
    outColor = vec4(result, 1.0);
}