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
    mat4 projView = uProjection * uView;
    vec2 aspectVec = vec2(uAspectRatio, 1.0);

    vec4 currentPosProjected = projView * vec4(aCurrentPos, 1.0);
    vec4 nextPosProjected = projView * vec4(aNextPos, 1.0);
    vec4 prevPosProjected = projView * vec4(aPreviousPos, 1.0);
    vec2 currentPosScreen = currentPosProjected.xy / currentPosProjected.w * aspectVec;
    vec2 nextPosScreen = nextPosProjected.xy / nextPosProjected.w * aspectVec;
    vec2 prevPosScreen = prevPosProjected.xy / prevPosProjected.w * aspectVec;

    // Normal of the line
    vec2 lineNormal = vec2(0.0);
    float thick = thickness;
    if (currentPosScreen == prevPosScreen) {
        // This is the starting point
        lineNormal = normalize(nextPosScreen - currentPosScreen);
    } else if (currentPosScreen == nextPosScreen) {
        // This is the ending point
        lineNormal = normalize(currentPosScreen - prevPosScreen);
    } else {
        // This is a point in the middle of the path
        //get directions from (C - B) and (B - A)
        vec2 dirA = normalize((currentPosScreen - prevPosScreen));
        vec2 dirB = normalize((nextPosScreen - currentPosScreen));
        //now compute the miter join normal and length
        vec2 tangent = normalize(dirA + dirB);
        vec2 perp = vec2(-dirA.y, dirA.x);
        vec2 miter = vec2(-tangent.y, tangent.x);
        lineNormal = tangent;
        float miterDotPerp = dot(miter, perp);
        thick = thickness / dot(miter, perp);
    }
    vec2 linePerpendicular = vec2(-lineNormal.y, lineNormal.x);
    linePerpendicular *= (thick / 2.0);
    linePerpendicular.x /= uAspectRatio;

    // Calculate the final vertex position
    gl_Position = currentPosProjected + vec4(linePerpendicular, 0.0, 0.0);
    fCenter = currentPosProjected.xy;
    fPos = gl_Position.xy;
    fThickness = abs(thick);
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
    FragColor = fColor;//vec4(fColor.rgb, (1.0 - d) * fColor.a);
}