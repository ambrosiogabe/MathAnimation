#type vertex
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in uvec2 aObjId;

out vec4 fColor;
out vec2 fTexCoord;
flat out uvec2 fObjId;

uniform mat4 uProjection;
uniform mat4 uView;

void main()
{
    fColor = aColor;
    fTexCoord = aTexCoord;
    fObjId = aObjId;
    gl_Position = uProjection * uView * vec4(aPos, 0.0, 1.0);
}

#type fragment
#version 330 core
layout (location = 0) out vec4 FragColor;
layout (location = 3) out uvec2 ObjId;

in vec4 fColor;
in vec2 fTexCoord;
flat in uvec2 fObjId;

uniform sampler2D uTexture;
uniform int uWireframeOn;

void main()
{
    // Just early out for wireframe
    if (uWireframeOn != 0) {
        FragColor = vec4(1);
        return;
    }

    vec4 texColor = texture(uTexture, fTexCoord);
    if (texColor.a < 0.05 || fColor.a < 0.05) {
        discard;
    }

    FragColor = fColor * texColor;
    ObjId = fObjId;
}