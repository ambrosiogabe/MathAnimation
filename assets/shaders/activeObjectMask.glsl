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
layout(location = 4) out vec4 FragColor;
layout(location = 5) out vec4 FragColor2;

in vec2 fTexCoords;

uniform usampler2D uObjectIdTexture;
uniform uvec2 uActiveObjectId;

void main()
{
    uvec2 sample = texture(uObjectIdTexture, fTexCoords).rg;
    if (sample == uActiveObjectId) {
        FragColor = vec4(fTexCoords, -1.0f, -1.0f);
        FragColor2 = vec4(fTexCoords, -1.0f, -1.0f);
    } else {
        discard;
    }
}