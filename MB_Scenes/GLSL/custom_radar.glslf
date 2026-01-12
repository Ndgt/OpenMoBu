
// animated radar effect

#version 140

in vec2 TexCoord;
out vec4 FragColor;

// Uniforms for animation and input texture
uniform float iTime;               // Current time in seconds
uniform sampler2D inputSampler;   // Input texture
uniform vec2 iResolution;          // Screen resolution (width, height)

void main() {
    // Calculate aspect ratio from resolution
    float aspectRatio = iResolution.x / iResolution.y;

    // Retrieve the texture color at this fragment
    vec4 texColor = texture(inputSampler, TexCoord);

    // Center of the radar
    vec2 center = vec2(0.5, 0.5);

    // Adjust coordinates for aspect ratio
    vec2 adjustedCoord = TexCoord;
    adjustedCoord.x *= aspectRatio;

    vec2 adjustedCenter = center;
    adjustedCenter.x *= aspectRatio;

    // Calculate the distance and angle relative to the center
    float distance = length(adjustedCoord - adjustedCenter);
    float angle = atan(adjustedCoord.y - adjustedCenter.y, adjustedCoord.x - adjustedCenter.x);

    // Radar sweep calculation
    float sweepAngle = mod(iTime * 2.0, 6.28318); // Sweeping angle, wraps every 2*PI
    float angleDifference = abs(angle - sweepAngle);

    // Smooth falloff for the radar line
    float lineIntensity = exp(-pow(angleDifference * 10.0, 2.0));

    // Circular radar rings
    float ringIntensity = 0.0;
    for (int i = 1; i <= 5; i++) {
        float ringRadius = float(i) * 0.1;
        ringIntensity += exp(-pow((distance - ringRadius) * 20.0, 2.0));
    }

    // Fade the rings with distance
    ringIntensity *= exp(-distance * 5.0);

    // Combine the radar effects
    vec3 radarEffect = vec3(0.0, lineIntensity + ringIntensity, 0.0); // Green radar effect

    // Output the final color
    FragColor = vec4(texColor.rgb + radarEffect, texColor.a);
}