#version 410
#define _CRT_SECURE_NO_WARNINGS
layout(location = 0) out vec4 fragColor;

uniform mat4 um4mv;
uniform mat4 um4p;

in VertexData
{
	vec3 N; // eye space normal
	vec3 N2;
	vec3 L; // eye space light vector
	vec3 V; // eye view vector
	vec3 V2;
	vec2 texcoord;
} vertexData;

uniform sampler2D tex;
uniform samplerCube tex_cubemap;

//object attribute
uniform vec3 ambient;
uniform vec3 diffuse;
uniform vec3 specular;
uniform float shininess;

//vec3 diffuse_albedo = vec3(0.35);
//vec3 specular_albedo = vec3(0.7);
float specular_power = 200.0;


//index
uniform int effect_index;

void main()
{
	//blinn phong
	vec3 N = normalize(vertexData.N);
	vec3 L = normalize(vertexData.L);
	vec3 V = normalize(vertexData.V);
	vec3 H = normalize(L + V);

	vec3 diffuse2 = diffuse * max(dot(N, L), 0.0);
	vec3 specular2 = pow(max(dot(N, H), 0.0), specular_power) * specular;

	vec4 bcolor = vec4(0.05 * ambient + diffuse2 + specular2, 1.0);

	//fragColor = 0.8 * bcolor + 0.2 * ecolor;

	//environment map
	vec3 r = reflect(vertexData.V2, normalize(vertexData.N2));
	vec4 ecolor = texture(tex_cubemap, r);

    //vec3 texColor = texture(tex,vertexData.texcoord).rgb;
    //fragColor = vec4(texColor, 1.0);
	fragColor = 0.8 * bcolor + 0.2 * ecolor;

	if (effect_index == 1)
	{
		fragColor = bcolor;
	}
	else if (effect_index == 2)
	{
		fragColor = ecolor;
	}
	else if (effect_index == 3)
	{
		fragColor = 0.8 * bcolor + 0.2 * ecolor;
	}

}