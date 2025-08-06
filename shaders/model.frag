#version 450

layout(binding = 1) uniform sampler2D texSampler;

layout(push_constant) uniform PushConstants {
    mat4 model;
    vec3 materialDiffuse;
    float hasTexture;
} pc;

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

void main() {
    // Improved lighting calculation with multiple lights
    vec3 normal = normalize(fragNormal);
    
    // Main directional light
    vec3 lightDir1 = normalize(vec3(-0.5, -1.0, -0.5));
    float diffuse1 = max(dot(-lightDir1, normal), 0.0);
    
    // Secondary fill light
    vec3 lightDir2 = normalize(vec3(0.3, -0.7, 0.4));
    float diffuse2 = max(dot(-lightDir2, normal), 0.0) * 0.4;
    
    // Rim lighting for better edge definition
    vec3 viewDir = normalize(-fragWorldPos);
    float rim = 1.0 - max(dot(normal, viewDir), 0.0);
    rim = pow(rim, 2.0) * 0.3;
    
    // Choose between texture and material color
    vec3 baseColor;
    if (pc.hasTexture > 0.5) {
        // Use texture if available
        baseColor = texture(texSampler, fragTexCoord).rgb;
    } else {
        // Use material diffuse color if no texture
        baseColor = pc.materialDiffuse;
    }
    
    // Enhanced lighting for better 3D appearance - increased ambient to prevent black surfaces
    vec3 finalColor = baseColor * (0.5 + diffuse1 * 1.2 + diffuse2 * 0.6 + rim * 0.8);
    outColor = vec4(finalColor, 1.0);
    
    // DEBUG: Uncomment ONLY ONE of these to test different outputs
    // outColor = vec4(abs(normal), 1.0);                    // Show normals as colors
    // outColor = vec4(baseColor, 1.0);                      // Pure texture/material color
}