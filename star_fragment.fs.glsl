#version 410 core

uniform float iTime;

out vec4 fragColor;

in VertexData
{
    vec3 N; // eye space normal
    vec3 L; // eye space light vector
    vec3 V; // eye space halfway vector
    vec2 texcoord;
	vec4 shadow_coord;
} vertexData;

void main()
{
	const int iterations = 17;
	const float formuparam = 0.53;

	const float volsteps = 18;
	const float stepsize = 0.050;

	const float zoom  = 0.800;
	const float tile  = 0.850;
	const float speed = 0.010;

	const float brightness = 0.0015;
	const float darkmatter = 0.300;
	const float distfading = 0.760;
	const float saturation = 0.800;

	vec2 iResolution = vec2(1440, 900);
	//get coords and direction
	vec2 uv = gl_FragCoord.xy/iResolution.xy-.5;
	uv.y*=iResolution.y/iResolution.x;
	vec3 dir=vec3(uv*zoom,1.);
	float time=iTime*speed+.25;

	
	vec3 from=vec3(1.,.5,0.5);
	
	
	vec3 forward = vec3(0.,0.,1.);
	
	//mouse rotation
	float a1 = 0.3;  //3.1415926 * (iMouse.x/iResolution.x-.5);
	mat2 rot1 = mat2(cos(a1),sin(a1),-sin(a1),cos(a1));
	float a2 = .6;   //3.1415926 * (iMouse.y/iResolution.y-.5);
	mat2 rot2 = mat2(cos(a2),sin(a2),-sin(a2),cos(a2));
	dir.xz*=rot1;
	forward.xz *= rot1;
	dir.yz*=rot1;
	forward.yz *= rot1;
	
	//zoom
	float zooom = iTime/20.;//4.*iMouse.y/iResolution.y + iTime/10.;
	from += forward* zooom;
	float sampleShift = mod( zooom, stepsize );
	float zoffset = -sampleShift;
	sampleShift /= stepsize; // make from 0 to 1
	
	//volumetric rendering
	float s=0.1;
	vec3 v=vec3(0.);
	for (int r=0; r<volsteps; r++) {
		vec3 p=from+(s+zoffset)*dir;// + vec3(0.,0.,zoffset);
		p = abs(vec3(tile)-mod(p,vec3(tile*2.))); // tiling fold
		float pa,a=pa=0.;
		for (int i=0; i<iterations; i++) { 
			p=abs(p)/dot(p,p)-formuparam; // the magic formula
			a+=abs(length(p)-pa); // absolute sum of average change
			pa=length(p);
		}
		//float dm=max(0.,darkmatter-a*a*.001); //dark matter
		a*=a*a; // add contrast
		//if (r>3) fade*=1.-dm; // dark matter, don't render near
		// brightens stuff up a bit
		float s1 = s+zoffset;
		// need closed form expression for this, now that we shift samples
		float fade = pow(distfading,max(0.,float(r)-sampleShift));
		v+=fade;
		// fade out samples as they approach the camera
		if( r == 0 )
			fade *= 1. - sampleShift;
		// fade in samples as they approach from the distance
		if( r == volsteps-1 )
			fade *= sampleShift;
		v+=vec3(2.*s1,4.*s1*s1,16.*s1*s1*s1*s1)*a*brightness*fade; // coloring based on distance
		s+=stepsize;
	}
	v=mix(vec3(length(v)),v,saturation); //color adjust
	fragColor = vec4(v*.01,1.);	


	// visualise image gradients. suppression using derivative instructions works well on some
	// speckles but i think the bottom right pixel in each quad doesnt contribute to the derivative (?)
	// and therefore wont be suppressed. I didnt get very far with this kind of approach. I
	// think temporal reprojection is the way to go.
	// http://casual-effects.blogspot.co.uk/2013/08/starfield-shader.html
	/*if( iMouse.z > 0. )
	{
		fragColor = clamp( fragColor, 0. , 1. );
	
		float intensity = fragColor.r + fragColor.g + fragColor.b;
		fragColor = vec4(fwidth(intensity));
	}*/
}
