#type vertex
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aNormal;

out vec4 fColor;
out vec2 fTexCoord;
out vec3 fNormal;
out vec3 fFragPos;

uniform mat4 uProjection;
uniform mat4 uView;
// uniform mat4 modelMatrix;

void main()
{
    fColor = aColor;
    fTexCoord = aTexCoord;
    fNormal = aNormal;
    gl_Position = uProjection * uView * vec4(aPos, 1.0);
}

#type fragment
#version 330 core
out vec4 FragColor;

in vec4 fColor;
in vec2 fTexCoord;
in vec3 fNormal;

uniform vec3 sunDirection;
uniform vec3 sunColor;

void main()
{
    float diff = max(dot(normalize(fNormal), sunDirection), 0.0);
    vec3 diffuse = diff * sunColor;
    vec3 ambient = vec3(0.1);

    FragColor = vec4(clamp(ambient + diffuse, vec3(0.0), vec3(1.0)), 1.0) * fColor;
}