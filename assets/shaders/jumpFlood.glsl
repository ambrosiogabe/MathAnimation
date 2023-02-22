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
out vec4 FragColor;

in vec2 fTexCoords;

uniform sampler2D uJumpMask;
uniform vec2 uSampleOffset;

vec2 offsets[9] = vec2[](
    vec2(-1.0f,  1.0f), vec2( 0.0f,  1.0f), vec2( 1.0f,  1.0f),
    vec2(-1.0f,  0.0f), vec2( 0.0f,  0.0f), vec2( 1.0f,  0.0f),
    vec2(-1.0f, -1.0f), vec2( 0.0f, -1.0f), vec2( 1.0f, -1.0f)
);

void main()
{
    float closestDistance = -1.0f;
    for (int i = 0; i < 9; i++) {
        vec4 pixelCoord = texture(uJumpMask, uSampleOffset + offsets[i]);
        if (pixelCoord.rg != vec2(-1.0f, -1.0f)) {
            float currentDistance = distance(uSampleOffset, vec2(0.0f));
            if (pixelCoord.b == -1.0f || currentDistance < pixelCoord.b) {
                FragColor = vec4(pixelCoord.rg, currentDistance, -1.0f);
            }
        }
    }
}