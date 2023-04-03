#type vertex
#version 330 core
layout (location = 0) in vec3 center;
layout (location = 1) in uint aColor;
layout (location = 2) in vec2 halfSize;

out vec4 fColor;
out vec2 fTexCoord;

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

    mat4 projView = uProjection * uView;
    vec4 centerProjected = projView * vec4(center, 1.0);

    // Offset of this vertex
    vec2 offset = halfSize;
    offset.x /= uAspectRatio;

    fTexCoord = (sign(halfSize) + vec2(1.0f)) / vec2(2.0f);

    // Calculate the final vertex position
    gl_Position = centerProjected + vec4(offset, 0.0, 0.0);
}

#type fragment
#version 330 core
layout(location = 0) out vec4 FragColor;

in vec4 fColor;
in vec2 fTexCoord;

uniform sampler2D uTexture;

void main()
{
    vec4 texColor = texture(uTexture, fTexCoord);

    if (texColor.a < 0.05 || fColor.a < 0.05) {
        discard;
    }

    FragColor = fColor * texColor;
}