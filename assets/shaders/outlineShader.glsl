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
layout(location = 0) out vec4 FragColor;

in vec2 fTexCoords;

uniform sampler2D uJumpMask;
uniform usampler2D uObjectIdTexture;

uniform float uOutlineWidth;
uniform vec4 uOutlineColor;
uniform vec2 uFramebufferSize;

uniform uvec2 uActiveObjectId;

#define UINT32_MAX uint(0xFFFFFFFF)

void main()
{
    uvec2 sample = texture(uObjectIdTexture, fTexCoords).rg;
    uvec2 maxUint = uvec2(UINT32_MAX, UINT32_MAX);

    vec2 normalizedSampleLocation = texture(uJumpMask, fTexCoords).rg;
    vec2 sampleLocation = normalizedSampleLocation * uFramebufferSize;
    vec2 currentPixel = fTexCoords * uFramebufferSize;
    float distanceToSample = distance(sampleLocation, currentPixel);
    if (distanceToSample < uOutlineWidth && sample == maxUint) {
        float alpha = clamp((uOutlineWidth - distanceToSample), 0.0f, 1.0f);
        FragColor = vec4(uOutlineColor.rgb, 1.0f);
    } else {
        FragColor = vec4(0.0f);
    }
}