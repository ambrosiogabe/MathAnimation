#type vertex
#version 330 core
layout (location = 0) in vec3 p0;
layout (location = 1) in float thickness;
layout (location = 2) in vec3 p1;
layout (location = 3) in uint aColor;
layout (location = 4) in uvec2 aObjId;

out vec4 fColor;
flat out uvec2 fObjId;

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

    vec4 p0Projected = projView * vec4(p0, 1.0);
    vec4 p1Projected = projView * vec4(p1, 1.0);

    // Normal of the line
    vec2 lineNormal = normalize(p1Projected.xy - p0Projected.xy);
    vec2 linePerpendicular = vec2(-lineNormal.y, lineNormal.x);
    linePerpendicular *= (thickness / 2.0);
    linePerpendicular.x /= uAspectRatio;

    fObjId = aObjId;

    // Calculate the final vertex position
    gl_Position = p0Projected + vec4(linePerpendicular, 0.0, 0.0);
}

#type fragment
#version 330 core
layout(location = 0) out vec4 FragColor;
layout (location = 3) out uvec2 ObjId;

in vec4 fColor;
flat in uvec2 fObjId;

#define UINT32_MAX uint(0xFFFFFFFF)

void main()
{
    // Do a different alpha threshold for object IDs
    if (fColor.a < 0.8) {
        ObjId = uvec2(UINT32_MAX, UINT32_MAX);
    } else {
        ObjId = fObjId;
    }

    FragColor = fColor;
}