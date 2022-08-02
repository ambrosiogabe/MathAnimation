#type vertex
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aTexCoord;

out vec4 fColor;
out vec2 fTexCoord;

uniform mat4 uProjection;
uniform mat4 uView;

void main()
{
    fColor = aColor;
    fTexCoord = aTexCoord;
    gl_Position = uProjection * uView * vec4(aPos, 0.0, 1.0);
}

#type fragment
#version 330 core
out vec4 FragColor;

in vec4 fColor;
in vec2 fTexCoord;

uniform sampler2D uTexture;

void main()
{
    FragColor = fColor * (float(texture(uTexture, fTexCoord).r) / 255.0f);
}