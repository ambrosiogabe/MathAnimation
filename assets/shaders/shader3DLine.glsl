#type vertex
#version 330 core
layout (location = 0) in vec3 aCurrentPos;
layout (location = 1) in vec3 aPreviousPos;
layout (location = 2) in vec3 aNextPos;
layout (location = 3) in float thickness;
layout (location = 4) in uint aColor;

out vec4 fColor;
out vec2 fPos;
out vec2 fCenter;
flat out float fThickness;

uniform mat4 uProjection;
uniform mat4 uView;
uniform float uAspectRatio;

void main()
{
    fColor = vec4(
        float(((aColor >> uint(24)) & uint(0xFF))) / 255.0f,
        float(((aColor >> uint(16)) & uint(0xFF))) / 255.0f,
        float(((aColor >> uint(8))  & uint(0xFF))) / 255.0f,
        float(aColor & uint(0xFF)) / 255.0f
    );

    // Expand the vertices according to the line's perpendicular directions
    vec2 aspectVec = vec2(uAspectRatio, 1.0);
    mat4 projView = uProjection * uView;
    vec4 currentPosProjected = projView * vec4(aCurrentPos, 1.0);
    vec2 currentPosScreen = currentPosProjected.xy / currentPosProjected.w * aspectVec;
    currentPosScreen.x *= uAspectRatio;

    // Normal of the line
    vec4 nextPosProjected = projView * vec4(aNextPos, 1.0);
    vec4 prevPosProjected = projView * vec4(aPreviousPos, 1.0);
    vec2 nextPosScreen = nextPosProjected.xy / nextPosProjected.w * aspectVec;
    vec2 prevPosScreen = prevPosProjected.xy / prevPosProjected.w * aspectVec;

    vec2 lineNormal = normalize(nextPosScreen - prevPosScreen);
    vec2 linePerpendicular = vec2(-lineNormal.y, lineNormal.x);
    linePerpendicular *= (thickness / 2.0);
    linePerpendicular.x /= uAspectRatio;

    // Calculate the final vertex position
    gl_Position = currentPosProjected + vec4(linePerpendicular, 0.0, 0.0);
    fCenter = currentPosProjected.xy;
    fPos = gl_Position.xy;
    fThickness = abs(thickness);
}

#type fragment
#version 330 core
out vec4 FragColor;

in vec4 fColor;
in vec2 fPos;
in vec2 fCenter;
flat in float fThickness;

void main()
{
    float d = pow(length(fPos - fCenter) / fThickness, 4);
    FragColor = vec4(fColor.rgb, (1.0 - d) * fColor.a);
}