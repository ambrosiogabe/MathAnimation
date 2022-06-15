#type vertex
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in uint aColor;
layout (location = 2) in uint aTexId;
layout (location = 3) in vec2 aTexCoord;

out vec4 fColor;
flat out uint fTexId;
out vec2 fTexCoord;

uniform mat4 uProjection;
uniform mat4 uView;

void main()
{
    fColor = vec4(
        float(((aColor >> uint(24)) & uint(0xFF))) / 255.0f,
        float(((aColor >> uint(16)) & uint(0xFF))) / 255.0f,
        float(((aColor >> uint(8))  & uint(0xFF))) / 255.0f,
        float(aColor & uint(0xFF)) / 255.0f
    );
    fTexCoord = aTexCoord;
    fTexId = aTexId;

    gl_Position = uProjection * uView * vec4(aPos, 1.0);

    // Remove the perspective
    gl_Position.xyz /= gl_Position.w;
}

#type fragment
#version 330 core
out vec4 FragColor;

in vec4 fColor;
flat in uint fTexId;
in vec2 fTexCoord;

void main()
{
    FragColor = fColor;
}