#version 410 core
out vec4 FragColor;

uniform ivec2 resolution;

in vec4 nearp;
in vec4 farp;

float sphere(vec3 p, vec4 circle){
	return length(p-circle.xyz) - circle.w;
}

float raymarch(vec3 ro, vec3 rd){
	 float di = 0;
	 for(int i = 0; i < 100; i++){
	 	vec3 p = ro + rd * di;
		float ds = 100;
		//for(int k = 0; k < rocks_count; k++){
			ds = min(ds, sphere(p,vec4(0,0,4,1)));
		//}
		//ds = min(ds,spaceShip(p));
		di += ds;
		if(di > 100.0 || di < 0.1)break;
	 }
	 return di;
}

void main()
{
	vec3 ro = nearp.xyz/nearp.w;
	vec3 rd = normalize((farp.xyz/farp.w) - ro);
	vec2 uv = (gl_FragCoord.xy - (0.5 * resolution.xy))/resolution.y;
	float d = raymarch(ro,rd);

	d /= 50;

	//if(d > 0.1)discard;
	FragColor = vec4(d);
	//FragColor = vec4(vec3(gl_FragCoord.z), 1.0);
}
