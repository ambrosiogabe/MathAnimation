#type vertex
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aNormal;
layout (location = 4) in uvec2 aObjId;

out vec4 fColor;
out vec2 fTexCoord;
out vec3 fNormal;
out vec3 fFragPos;
flat out uvec2 fObjId;

uniform mat4 uProjection;
uniform mat4 uView;
// uniform mat4 modelMatrix;

void main()
{
    fColor = aColor;
    fTexCoord = aTexCoord;
    fNormal = aNormal;
    fObjId = aObjId;
    gl_Position = uProjection * uView * vec4(aPos, 1.0);
}

#type fragment
#version 330 core
layout (location = 0) out vec4 FragColor;
layout (location = 3) out uvec2 ObjId;

in vec4 fColor;
in vec2 fTexCoord;
in vec3 fNormal;
flat in uvec2 fObjId;

uniform vec3 sunDirection;
uniform vec3 sunColor;
uniform sampler2D uTexture;

void main()
{
    vec4 textureColor = texture(uTexture, fTexCoord) * fColor;
    if (textureColor.a < 0.5) 
    {
        discard;
    }

    float diff = max(dot(normalize(fNormal), sunDirection), 0.0);
    vec3 diffuse = diff * sunColor * textureColor.rgb;
    vec3 ambient = vec3(0.1);

    //FragColor = vec4(clamp(ambient + diffuse, vec3(0.0), vec3(1.0)), 1.0) * fColor;
    FragColor = textureColor;
    ObjId = fObjId;
}