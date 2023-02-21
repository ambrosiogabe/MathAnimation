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

uniform vec4 uColor;

void main()
{
    FragColor = uColor;
    FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}