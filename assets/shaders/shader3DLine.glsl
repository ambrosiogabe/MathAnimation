#type vertex
#version 330 core
layout (location = 0) in vec3 aCurrentPos;
layout (location = 1) in vec3 aPreviousPos;
layout (location = 2) in vec3 aNextPos;
layout (location = 3) in float thickness;
layout (location = 4) in uint aColor;

out vec4 fColor;

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
        vec2 dirA = normalize(currentPosScreen - prevPosScreen);
        vec2 dirB = normalize(nextPosScreen - currentPosScreen);
        //now compute the miter join normal and length
        vec2 bisection = normalize(dirA + dirB);
        vec2 firstLinePerp = vec2(-dirA.y, dirA.x);
        // This is the miter
        vec2 bisectionPerp = vec2(-bisection.y, bisection.x);
        lineNormal = bisection;
        thick = thickness / dot(bisectionPerp, firstLinePerp);
        // TODO: Right now we're limiting the thickness so that 
        // we don't get artifacts when one of the lines has 0
        // thickness because it's perpendicular to the camera's
        // view angle. In the future, we can switch to a bevel
        // join here to fix that instead.
        thick = max(min(abs(thickness), thick), -abs(thickness));
    }
    vec2 linePerpendicular = vec2(-lineNormal.y, lineNormal.x);
    linePerpendicular *= (thick / 2.0);
    linePerpendicular.x /= uAspectRatio;

    // Calculate the final vertex position
    gl_Position = currentPosProjected + vec4(linePerpendicular, 0.0, 0.0);
}

#type fragment
#version 330 core
layout(location = 0) out vec4 FragColor;

in vec4 fColor;

void main()
{
    FragColor = fColor;
}