#version 450 core 

// I think these are auto generated by opengl with it's using draw triangles 
layout (location = 0) in vec3 i_Pos; 
layout (location = 1) in vec2 i_Uv; 

out vec2 f_Uv; 

void main() 
{
    f_Uv  = i_Uv;
    gl_Position = vec4(i_Pos, 1.0);
}