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
layout(location = 0) out uint UColor;
layout(location = 1) out uint VColor;

in vec2 fTexCoords;

uniform sampler2D uTexture;

void main()
{
	vec4 color = texture(uTexture, fTexCoords);
    // Taken from https://en.wikipedia.org/wiki/YCbCr#RGB_conversion
    // Y'CbCr from R'dG'dB'd
    float u = (-37.797f * color.r / 255.0f) 
        - (74.203f * color.g / 255.0f) 
        + (112.0f * color.b / 255.0f) 
        + (128.0f / 255.0f);
    float v = (112.0f * color.r / 255.0f) 
        - (93.786f * color.g / 255.0f) 
        - (18.214f * color.b / 255.0f) 
        + (128.0f / 255.0f);

    UColor = uint(u * 255.0f);
    VColor = uint(v * 255.0f);
}