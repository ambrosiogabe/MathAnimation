#ifndef VECTOR_SHADER_GLSL_H
#define VECTOR_SHADER_GLSL_H

static const char* vectorShaderGlsl = R"(
#type vertex
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec4 aColor;

// See https://www.microsoft.com/en-us/research/wp-content/uploads/2005/01/p1000-loop.pdf
// for more information. (Resolution Independent Curve Rendering using Programmable
// Graphics Hardware by Charles Loop and Jim Blinn)
//
// These values are equal to the dot product of the Bezier control points
// and the tangent lines to the cubic bezier k, l, m and n. In other words:
//
//    aProcedrualTexCoords.x = dot(bezierControlPoints.xy, k.xy);
//    aProceduralTexCoords.y = dot(bezierControlPoints.xy, l.xy);
//    aProceduralTexCoords.z = dot(bezierControlPoints.xy, m.xy);
//    aProceduralTexCoords.w = dot(bezierControlPoints.xy, n.xy);
//
// We can then use these procedural tex coords to determine if a point
// is inside or outside the cubic bezier curve by testing:
//
//    pow(k, 3) - l * m * n > 0
//
// which is equivalent to:
//
//   pow(aProceduralTexCoords.x, 3) -
//      aProceduralTexCoords.y * aProceduralTexCoords.z * aProceduralTexCoords.w > 0
//
layout (location = 2) in vec2 aProceduralTexCoords;
layout (location = 3) in int aIsConcave;
layout (location = 4) in int aBezierIndex;

uniform mat4 uProjection;
uniform mat4 uView;

out vec2 fUv;
out vec4 fColor;
flat out int fIsConcave;
flat out int fBezierIndex;

void main() {
    fUv = aProceduralTexCoords;
    fColor = aColor;
    fIsConcave = aIsConcave;
    fBezierIndex = aBezierIndex;

    gl_Position = uProjection * uView * vec4(aPos.x, aPos.y, 0.0, 1.0);
}

#type fragment
#version 330 core

layout (location = 0) out vec4 FragColor;

in vec2 fUv;
in vec4 fColor;
flat in int fIsConcave;
flat in int fBezierIndex;

void main() {
    // bool inside = fUv.x * fUv.x - fUv.y < 0;
    // These two are equivalent
    bool inside = fIsConcave == 0
        ? fUv.x * fUv.x * fUv.x - fUv.x * fUv.y < 0
        : fUv.x * fUv.x * fUv.x - fUv.x * fUv.y > 0;
    if (!inside) {
        discard;
    }

    FragColor = vec4(fColor.rgb, 1.0);
}
)";

#endif 