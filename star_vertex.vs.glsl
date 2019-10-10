#version 410 core

layout(location = 0) in vec3 iv3vertex;
layout(location = 1) in vec2 iv2tex_coord;
layout(location = 2) in vec3 iv3normal;

uniform mat4 um4mv;
uniform mat4 um4p;
uniform mat4 um4mvp;
uniform mat4 shadow_matrix;

out VertexData
{
    vec3 N; // eye space normal
    vec3 L; // eye space light vector
    vec3 V; // eye space halfway vector
    vec2 texcoord;
	vec4 shadow_coord;
} vertexData;

uniform vec3 light_pos = vec3(-31.75f, 26.05f, -97.72f);

void main()
{

	

	// for environment mapping.
	vec4 pos_vs = um4mv * vec4(iv3vertex, 1.0);
	vertexData.N = mat3(um4mv) * iv3normal;
	vertexData.L = light_pos - pos_vs.xyz;
	vertexData.V = pos_vs.xyz;

	vertexData.shadow_coord = shadow_matrix * vec4(iv3vertex, 1.0f);

	gl_Position = um4p * um4mv * vec4(iv3vertex, 1.0);
    vertexData.texcoord = iv2tex_coord;
}