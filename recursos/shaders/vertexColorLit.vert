#version 420 core

$GLMatrices

uniform vec4 lightpos; // lightpos (in eye space)

in vec4 position;
in vec3 normal;
in vec4 color;

out vec4 vertexColor;

void main()
{

	vec4 vertexPos = modelviewMatrix * position;
	vec3 lightDir = normalize(vec3(lightpos-vertexPos));
	vec3 n = normalize(normalMatrix * normal);
	float intensity = max(dot(lightDir,n),0.1);

	vertexColor = intensity * color;
	vertexColor.a = color.a;
	gl_Position = projMatrix * vertexPos;
}