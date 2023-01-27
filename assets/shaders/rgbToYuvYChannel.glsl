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

in vec2 fTexCoords;

uniform sampler2D uTexture;

void main()
{
	vec4 color = texture(uTexture, fTexCoords);
    // Taken from https://en.wikipedia.org/wiki/YCbCr#RGB_conversion
    // Y'CbCr from R'dG'dB'd
    float y = (65.481f * color.r / 255.0f) 
        + (128.553f * color.g / 255.0f) 
        + (24.966f * color.b / 255.0f) 
        + (16.0f / 255.0f);

    YColor = uint(y * 255.0f);
}