#version 430 core

layout(location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    TexCoords = aPos;

    // strip translation from view on the C++ side,
    // here we just do the usual projection * view * pos
    vec4 pos = projection * view * vec4(aPos, 1.0);

    // force depth = 1 so the cube stays at the far plane
    gl_Position = vec4(pos.xy, pos.w, pos.w);
}