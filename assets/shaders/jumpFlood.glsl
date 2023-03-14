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
    vec2 closestSample = vec2(-1.0f, -1.0f);
    for (int i = 0; i < 9; i++) {
        vec2 sampleLocation = fTexCoords + uSampleOffset * offsets[i];
        vec4 pixelCoord = texture(uJumpMask, sampleLocation);
        if (pixelCoord.rg != vec2(-1.0f, -1.0f)) {
            float currentDistance = distance(pixelCoord.rg, fTexCoords);
            if (closestDistance == -1.0f || currentDistance < closestDistance) {
                closestDistance = currentDistance;
                closestSample = pixelCoord.rg;
            }
        }
    }

    FragColor = vec4(closestSample, -1.0f, -1.0f);
}