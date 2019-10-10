#version 430

layout(location = 0) out vec4 scolor;

in VS_OUT                              
{                                          
    vec3    tc;                            
} fs_in;

uniform samplerCube tex_cubemap;


void main()
{
	scolor = texture(tex_cubemap, fs_in.tc);
}