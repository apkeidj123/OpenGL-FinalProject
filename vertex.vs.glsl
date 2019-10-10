#version 410
#define _CRT_SECURE_NO_WARNINGS
layout(location = 0) in vec3 iv3vertex;
layout(location = 1) in vec2 iv2tex_coord;
layout(location = 2) in vec3 iv3normal;

uniform mat4 um4mv;
uniform mat4 um4p;

out VertexData
{
	vec3 N; // eye space normal
	vec3 N2;
	vec3 L; // eye space light vector
	vec3 V; // eye space halfway vector
	vec3 V2;
	vec2 texcoord;
} vertexData;

uniform vec3 light_pos = vec3(-50.0, 0.0, -100.0);

void main()
{

	//blinn phong
	vec4 P = um4mv * vec4(iv3vertex, 1.0);
	vertexData.N = mat3(um4mv) * iv3normal;
	vertexData.L = light_pos - P.xyz;
	vertexData.V = -P.xyz;

	//environment map
	vertexData.N2 = mat3(um4mv) * iv3normal;
	vertexData.V2 = P.xyz;

	gl_Position = um4p * P;

	//instance
	mat4 offset;
	offset[0] = vec4(1.0, 0.0, 0.0, 0.0);
	offset[1] = vec4(0.0, 1.0, 0.0, 0.0);
	offset[2] = vec4(0.0, 0.0, 1.0, 0.0);
	offset[3] = vec4((floor(gl_InstanceID / 10.0) - 4.5) * 20.0, 0.0, (mod(gl_InstanceID, 10) - 4.5) * 20.0, 1.0);

	//gl_Position = um4p * offset * P;

	vertexData.texcoord = iv2tex_coord;
}