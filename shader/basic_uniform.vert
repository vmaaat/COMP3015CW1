#version 460

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec2 VertexTexCoord;
layout (location = 2) in vec3 VertexNormal;

out vec2 TexCoord;
out vec3 FragPos;
out vec3 Normal;

uniform mat4 RotationMatrix; // we use this as our model matrix

void main()
{
    // World-space position of the fragment (really model-space, since we have no view/projection)
    vec4 worldPos = RotationMatrix * vec4(VertexPosition, 1.0);
    FragPos = worldPos.xyz;

    // Properly transform normals
    mat3 normalMatrix = mat3(transpose(inverse(RotationMatrix)));
    Normal = normalize(normalMatrix * VertexNormal);

    TexCoord = VertexTexCoord;

    // No projection matrix yet – treat model space as clip space
    gl_Position = worldPos;
}