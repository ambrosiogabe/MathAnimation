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
flat out uvec2 fObjId;

uniform mat4 uProjection;
uniform mat4 uView;

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

layout (location = 1) out vec4 accumulation;
layout (location = 2) out float revealage;
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
   float diff = max(dot(normalize(fNormal), sunDirection), 0.0);
   vec3 diffuse = diff * sunColor;
   vec3 ambient = vec3(0.1);

   vec4 objColor = fColor * texture(uTexture, fTexCoord);
   //vec4 premultipliedReflect = vec4(clamp(ambient + diffuse, vec3(0.0), vec3(1.0)), 1.0) * objColor;
   vec4 premultipliedReflect = objColor;

   /* Modulate the net coverage for composition by the transmission. This does not affect the color channels of the
      transparent surface because the caller's BSDF model should have already taken into account if transmission modulates
      reflection. This model doesn't handled colored transmission, so it averages the color channels. See

         McGuire and Enderton, Colored Stochastic Shadow Maps, ACM I3D, February 2011
         http://graphics.cs.williams.edu/papers/CSSM/

      for a full explanation and derivation.*/

   // premultipliedReflect.a *= 1.0 - clamp(0.5, 0.0, 1.0);

   /* You may need to adjust the w function if you have a very large or very small view volume; see the paper and
      presentation slides at http://jcgt.org/published/0002/02/09/ */
   // Intermediate terms to be cubed
   float a = min(1.0, premultipliedReflect.a) * 8.0 + 0.01;
   float b = -gl_FragCoord.z * 0.95 + 1.0;

   /* If your scene has a lot of content very close to the far plane,
      then include this line (one rsqrt instruction):
      b /= sqrt(1e4 * abs(csZ)); */
   float w = clamp(a * a * a * 1e8 * b * b * b, 1e-2, 3e2);
   accumulation = premultipliedReflect * w;
   revealage = premultipliedReflect.a;

   if (w > 0.5) {
      ObjId = fObjId;
   } else {
      ObjId = uvec2(0xFFFFFFFFUL, 0xFFFFFFFFUL);
   }
}