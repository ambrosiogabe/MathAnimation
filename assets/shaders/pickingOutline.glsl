#type vertex
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in highp vec2 aTexCoords;

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

uniform usampler2D uObjectIdTexture;
uniform uint uActiveObjectId;
uniform vec2 uResolution;
//uniform float uThreshold;

void main()
{
    // I don't even know what I'm doing here, but it kind of works...
    // It's supposed to be calculating the change of rate in the 
    // object id textures. If there's at least one correct sample
    // and the change in rate is greater than 0 then it detects
    // the "edge" of the object and renders it yellow
    float radius = 1.3;
    vec2 offsetSize = vec2(1.0 / uResolution.x, 1.0 / uResolution.y) * radius;
    vec2 offsets[9] = vec2[](
        vec2(-1.0, 1.0),  // top-left
        vec2( 0.0, 1.0),  // top-center
        vec2( 1.0, 1.0),  // top-right
        vec2(-1.0, 0.0),  // center-left
        vec2( 0.0, 0.0),  // center-center
        vec2( 1.0, 0.0),  // center-right
        vec2(-1.0, -1.0), // bottom-left
        vec2( 0.0, -1.0), // bottom-center
        vec2( 1.0, -1.0)  // bottom-right 
    );

    float sobelXKernel[9] = float[](
        1.0, 0.0, -1.0,
        2.0, 0.0, -2.0,
        1.0, 0.0, -1.0
    );

    float sobelYKernel[9] = float[](
        1.0, 2.0, 1.0,
        0.0, 0.0, 0.0,
        -1.0, -2.0, -1.0
    );

    float sobelX[9];
    float sobelY[9];
    float gradientX = 0.0;
    float gradientY = 0.0;
    int numCorrectSamples = 0;
    for (int i = 0; i < 9; i++) {
        uint sample = texture(uObjectIdTexture, fTexCoords + (offsets[i] * offsetSize)).r;
        // Make sure not to use UINT32_MAX so that precision doesn't get messed up
        float fSample = sample == 0xFFFFFFFFUL ? float(0xFFFFFUL) : sample;
        gradientX += float(sample) * sobelXKernel[i];
        gradientY += float(sample) * sobelYKernel[i];
        numCorrectSamples += int(sample == uActiveObjectId);
    }
    float gradient = sqrt(gradientX * gradientX + gradientY * gradientY);
    //uint sample = texture(uObjectIdTexture, fTexCoords).r;
    //float fSample = sample == 0xFFFFFFFFUL ? float(0xFFFFFUL) : sample;
    //float gradient = fwidth(gradient);

    const float threshold = -0.5;
    if (gradient > 0.0 && numCorrectSamples > 0) {
        FragColor = vec4(1, 1, 0, 1);
    } else {
        discard;
    }
}