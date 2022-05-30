#ifndef DEFAULT_SHADER_GLSL_H
#define DEFAULT_SHADER_GLSL_H

static const char* defaultShaderGlsl = R"(
#type vertex
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in uint aTexId;
layout (location = 3) in vec2 aTexCoord;

out vec4 fColor;
flat out uint fTexId;
out vec2 fTexCoord;

uniform mat4 uProjection;
uniform mat4 uView;

void main()
{
    fColor = aColor;
    fTexCoord = aTexCoord;
    fTexId = aTexId;
    gl_Position = uProjection * uView * vec4(aPos.x, aPos.y, 0.0, 1.0);
}

#type fragment
#version 330 core
#define numTextures 8
out vec4 FragColor;

in vec4 fColor;
flat in uint fTexId;
in vec2 fTexCoord;

uniform usampler2D uFontTextures[numTextures];

vec4 getSampleFromFont(int index, vec2 uv) {
    uint rVal = uint(0);
    for (int i = 0; i < numTextures; ++i) {
      uint c = texture(uFontTextures[i], uv).r;
      if (i == index) {
        rVal += c;
      }
    }
    return vec4(float(rVal) / 255.0, float(rVal) / 255.0, float(rVal) / 255.0, float(rVal) / 255.0);
}

void main()
{
    if (int(fTexId) == 0)
    {
	    FragColor = fColor;
    }
    else
    {
        FragColor = getSampleFromFont(int(fTexId) - 1, fTexCoord) * fColor;
    }
}
)";

#endif 