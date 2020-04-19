#version 330 core

uniform float time;

in vec3 interpolatedNormal;
in vec2 st;
in float len; 

out vec4 color;

void main() {
	vec3 watercolor = vec3(0.0, 0.0, 1.0);
	vec3 cloudcolor = vec3(0.1, 0.1, 0.9);
	//float alpha = sin(len*time);
	
	//vec3 diffusecolor =  mix(watercolor, cloudcolor, alpha);
	vec3 diffusecolor =  watercolor;

	vec3 nNormal = normalize(interpolatedNormal);
	float diffuselighting = max(0.0, nNormal.z);

	color = vec4(diffusecolor*diffuselighting, 1.0);
}
