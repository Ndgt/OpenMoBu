
// screen space reflections (based on mask A)

#version 140

in vec2 TexCoord;
out vec4 FragColor;

// Uniforms
uniform sampler2D inputSampler;    // The main rendered scene texture
uniform sampler2D depthSampler;    // Depth texture of the scene
uniform sampler2D maskSampler; // Mask texture for reflectivity
//uniform sampler2D normalTexture;   // World-space normals texture
uniform vec2 iResolution;
uniform mat4 projection;     // Projection matrix
uniform mat4 modelView;           // View matrix
uniform vec3 cameraPosition;       // Camera position in world space
uniform int maxSteps;              // Maximum steps for ray marching (default 50)
uniform float stepSize;            // Step size for ray marching (default 0.1)
uniform float reflectionStrength;  // Reflection intensity (default 0.5)

vec3 computeNormalFromDepth(vec2 uv) {
    vec2 texelSize = 1.0 / iResolution;
    float depthCenter = texture(depthSampler, uv).r;
    float depthRight = texture(depthSampler, uv + vec2(texelSize.x, 0.0)).r;
    float depthUp = texture(depthSampler, uv + vec2(0.0, texelSize.y)).r;

    vec3 dx = vec3(texelSize.x, 0.0, depthRight - depthCenter);
    vec3 dy = vec3(0.0, texelSize.y, depthUp - depthCenter);

    return normalize(cross(dx, dy));
}

// Helper functions
vec3 reconstructPosition(vec2 uv, float depth) {
    // Convert depth to NDC (Normalized Device Coordinates)
    vec4 ndc = vec4(uv * 2.0 - 1.0, depth, 1.0);
    // Reconstruct world position
    vec4 clipSpace = inverse(projection) * ndc;
    vec3 worldPosition = (clipSpace / clipSpace.w).xyz;
    return worldPosition;
}

void main() {
	//vec2 TexCoord = gl_TexCoord[0].st;
    // Fetch depth and normal at the current fragment
    float depth = texture(depthSampler, TexCoord).r;
    //vec3 normal = texture(normalTexture, TexCoord).rgb * 2.0 - 1.0;
	vec3 normal = computeNormalFromDepth(TexCoord);

	// Fetch reflectivity from the mask texture
    float reflectivity = texture(maskSampler, TexCoord).r;

    // If reflectivity is very low, skip SSR and use the original scene color
    if (reflectivity < 0.01) {
        FragColor = texture(inputSampler, TexCoord);
        return;
    }

    // Reconstruct the world position of the fragment
    vec3 worldPosition = reconstructPosition(TexCoord, depth);

    // Compute view direction
    vec3 viewDir = normalize(worldPosition - cameraPosition);

    // Reflect view direction using the fragment normal
    vec3 reflectionDir = reflect(viewDir, normal);

    // Start ray-marching
    vec3 rayOrigin = worldPosition;
    vec3 rayStep = reflectionDir * stepSize;
    vec2 uv = TexCoord;

    for (int i = 0; i < maxSteps; i++) {
        // Advance the ray
        rayOrigin += rayStep;

        // Project ray origin back to screen space
        vec4 clipPos = projection * modelView * vec4(rayOrigin, 1.0);
        vec3 ndcPos = clipPos.xyz / clipPos.w;
        uv = ndcPos.xy * 0.5 + 0.5;

        // Check if UV is outside the screen
        if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) break;

        // Sample depth at the new position
        float sampledDepth = texture(depthSampler, uv).r;
        vec3 sampledWorldPos = reconstructPosition(uv, sampledDepth);

        // Check for intersection
        if (sampledWorldPos.z > rayOrigin.z) {
            // Compute reflection color and blend
            vec3 reflectedColor = texture(inputSampler, uv).rgb;
            FragColor = vec4(reflectedColor * reflectionStrength, 1.0);
            return;
        }
    }

    // Fallback to the original color if no reflection found
    FragColor = texture(inputSampler, TexCoord);
}