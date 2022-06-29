#type vertex
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 fTexCoords;

void main()
{
	fTexCoords = aTexCoords;
	gl_Position = vec4(aPos, 0.0, 1.0);
}

#type fragment
#version 330 core
layout(location = 0) out vec4 FragColor;

in vec2 fTexCoords;

uniform sampler2D uAccumTexture;
uniform sampler2D uRevealageTexture;

void main()
{
    ivec2 textureCoords = ivec2(gl_FragCoord.xy);
    float revealage = texelFetch(uRevealageTexture, textureCoords, 0).r;
    if (revealage == 1.0) {
        // Save the blending and color texture fetch cost
        discard; 
    }

    vec4 accum = texelFetch(uAccumTexture, textureCoords, 0);
    // Suppress overflow
    if (isinf(max(
            max(
                max(abs(accum.r), abs(accum.g)), 
            accum.b),
        accum.a)
    )) {
        accum.rgb = vec3(accum.a);
    }
    vec3 averageColor = accum.rgb / max(accum.a, 0.00001);

    // dst' =  (accum.rgb / accum.a) * (1 - revealage) + dst * revealage
    FragColor = vec4(averageColor, 1.0 - revealage);
}