#type vertex
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 fTexCoords;

void main()
{
	fTexCoords = aTexCoords;
	gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
}

#type fragment
#version 330 core
layout(location = 0) out uint YColor;
layout(location = 1) out uint UColor;
layout(location = 2) out uint VColor;

in vec2 fTexCoords;

uniform sampler2D uTexture;

void main()
{
	vec4 color = texture(uTexture, fTexCoords);
    // Taken from https://stackoverflow.com/a/17934865
    float y = 0.257f * color.r + 0.504f * color.g + 0.098f * color.b + (16.0f / 255.0f);
    float u = -0.148f * color.r - 0.291f * color.g + 0.439f * color.b + (128.0f / 255.0f);
    float v = 0.439f * color.r - 0.368f * color.g - 0.071f * color.b + (128.0f / 255.0f);

    YColor = uint(y * 255.0f);
    UColor = uint(u * 255.0f);
    VColor = uint(v * 255.0f);
}