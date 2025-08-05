#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

// Fullscreen quad vertices
vec2 positions[6] = vec2[](
    vec2(-1.0, -1.0),
    vec2( 1.0, -1.0),
    vec2( 1.0,  1.0),
    vec2(-1.0, -1.0),
    vec2( 1.0,  1.0),
    vec2(-1.0,  1.0)
);

layout(location = 0) out vec3 nearPoint;
layout(location = 1) out vec3 farPoint;
layout(location = 2) out mat4 fragView;
layout(location = 6) out mat4 fragProj;

vec3 UnprojectPoint(float x, float y, float z, mat4 view, mat4 projection) {
    mat4 viewInv = inverse(view);
    mat4 projInv = inverse(projection);
    vec4 unprojectedPoint = viewInv * projInv * vec4(x, y, z, 1.0);
    return unprojectedPoint.xyz / unprojectedPoint.w;
}

void main() {
    vec2 p = positions[gl_VertexIndex];
    
    // Unproject to get world space points
    nearPoint = UnprojectPoint(p.x, p.y, 0.0, ubo.view, ubo.proj); // Near plane
    farPoint = UnprojectPoint(p.x, p.y, 1.0, ubo.view, ubo.proj);  // Far plane
    
    fragView = ubo.view;
    fragProj = ubo.proj;
    
    gl_Position = vec4(p, 0.0, 1.0); // NDC
}