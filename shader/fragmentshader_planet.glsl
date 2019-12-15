#version 330 core

uniform float time;

in vec3 interpolatedNormal;
in vec2 st;
in float len; 

out vec4 color;

void main() {
	vec3 groundColor = vec3(0.7, 0.5, 0.3);
	vec3 grassColor = vec3(0.0, 1.0, 0.0);
	vec3 snowColor = vec3(1.0, 1.0, 1.0);

	float grassLevel = 0.98;
	float groundlevel = 1.05;
	float snowLevel = 1.2;
	
	float alpha;
	vec3 diffusecolor; 
	if (len > groundlevel) {
		alpha = (snowLevel - len) / (snowLevel - groundlevel);
		diffusecolor = mix(snowColor, groundColor, alpha);
	}
	else {
		alpha = (groundlevel - len) / (groundlevel - grassLevel);
		diffusecolor = mix(groundColor, grassColor, alpha);
	}

	vec3 nNormal = normalize(interpolatedNormal);
	float diffuselighting = max(0.0, nNormal.z);

	color = vec4(diffusecolor*diffuselighting, 1.0);
}
