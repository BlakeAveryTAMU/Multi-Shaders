#version 120

uniform vec3 lightPos1;
uniform vec3 lightPos2;
uniform vec3 lightColor1;
uniform vec3 lightColor2;
uniform vec3 ka;
uniform vec3 kd;
uniform vec3 ks;
uniform float s;

varying vec3 vPos; // camera space position
varying vec3 vNor; // camera space normal

void main()
{

	//cd1: 
	vec3 lightDir1 = lightPos1 - vPos;
	lightDir1 = normalize(lightDir1);
	float lambertian1 = max(0.0, dot(lightDir1, normalize(vNor)));

	//cs1:
	vec3 eyeVector = normalize(-1 * vPos);
	vec3 halfDir1 = normalize(lightDir1 + eyeVector);
	float specular1 = pow(max(0.0, dot(halfDir1, normalize(vNor))), s);

	//cd2:
	vec3 lightDir2 = lightPos2 - vPos;
	lightDir2 = normalize(lightDir2);
	float lambertian2 = max(0.0, dot(lightDir2, normalize(vNor)));

	//cs2:
	vec3 halfDir2 = normalize(lightDir2 + eyeVector);
	float specular2 = pow(max(0.0, dot(halfDir2, normalize(vNor))), s);

	vec3 cd1 = kd * lambertian1;
	vec3 cs1 = ks * specular1;

	vec3 cd2 = kd * lambertian2;
	vec3 cs2 = ks * specular2;


	vec3 color1 = lightColor1 * (ka + cd1 + cs1);
	vec3 color2 = lightColor2 * (ka + cd2 + cs2);
	vec3 color = color1 + color2;
	gl_FragColor = vec4(color, 1.0);
	
	
}
