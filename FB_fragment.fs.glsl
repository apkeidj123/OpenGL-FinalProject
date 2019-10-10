#version 410 core

uniform sampler2D tex1;
uniform sampler2D tex2;
uniform sampler2D tex3;
uniform sampler2D tex4;

uniform int flag;
uniform int shader_index;

out vec4 color;

in VS_OUT
{
	vec2 texcoord;
} fs_in;

vec2 img_size = vec2(1440, 900);

void main()
{
	vec3 texture_1 = texture(tex1, fs_in.texcoord).rgb;
	vec3 texture_2 = texture(tex2, fs_in.texcoord).rgb;
	vec3 texture_3 = texture(tex3, fs_in.texcoord).rgb;
	vec3 texture_4 = texture(tex4, fs_in.texcoord).rgb;

	switch(flag) {
		case 0:
			if(texture_3.x == 1.0 && texture_3.y == 1.0 && texture_3.z == 1.0) {
				color = vec4(texture_1, 1.0); 
			} else {
				color = vec4(texture_3, 1.0); 
			}
			break;
		case 1:
			if(texture_3.x == 1.0 && texture_3.y == 1.0 && texture_3.z == 1.0) {
				color = vec4(texture_2, 1.0); 
			} else {
				color = vec4(texture_3, 1.0); 
			}
			break;
		case 2:
			if(texture_3.x == 1.0 && texture_3.y == 1.0 && texture_3.z == 1.0) {
				color = vec4(texture_2, 1.0); 
			} else {
				color = vec4(texture_3, 1.0); 
			}
			break;
		case 3:
			color = vec4(texture_4, 1.0);
			break;
		default:
			color = vec4(texture_4, 1.0);
			break;
	}

}