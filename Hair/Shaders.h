#pragma once

const char* FragmentShaderHair =
{
	// basic
	"#version 330\n"

	"out vec4 color;"

	"in vec3 normalF;"
	"in vec3 eyeDir;"

	"void main()"
	"{"
	"vec3 lightDir = normalize(vec3(1,2,1.5f));"
	"float sinLT = length(cross(lightDir, normalize(normalF)));"
	"float sinET = length(cross(eyeDir, -normalF));"
	"float cosLT = dot(lightDir, normalF);"
	"float cosET = dot(eyeDir, -normalF);"
	"float diffuse = clamp(sinLT, 0, 1);"
	"vec4 diffuseColor = vec4(0.75f,0.65f,0,1);"
	"vec4 diffComponent = diffuseColor * diffuse;"
	"float specular = clamp(cosLT * cosET + sinLT * sinET, 0, 1);"
	"specular = pow(specular, 80);"
	"vec4 specularColor = vec4(0.5f,0.5f,0.5f,1);"
	"vec4 specComponent = specularColor * specular;"
	"vec4 ambientComponent = diffuseColor * 0.1f;"
	"color = (diffComponent + specComponent) * sinLT + ambientComponent;"
	"color.a = 1;"
	"}"
};


const char* FragmentShaderHead =
{
	// basic
	"#version 330\n"

	"out vec4 color;"

	"in vec3 normalF;"
	"in vec3 eyeDir;"

	"void main()"
	"{"
	"vec3 lightDir = normalize(vec3(1,2,1.5f));"
	"float diffuse = clamp(dot(lightDir, normalF), 0, 1);"
	"vec4 diffuseColor = vec4(1.0,0.9f,.8f,1);"
	"vec4 diffComponent = diffuseColor * diffuse;"
	"vec4 ambientComponent = diffuseColor * 0.1f;"
	"color = (diffComponent) * diffuse + ambientComponent;"
	"color.a = 1;"
	"}"
};

const char* VertexShaderMesh =
{
	"#version 330\n"

	"uniform mat4 model;"
	"uniform mat4 vp;"
	"uniform vec3 eyePos;"

	"layout(location = 0) in vec3 vertexPosition;"
	"layout(location = 1) in vec3 vertexNormal;"

	"out vec3 normalF;"
	"out vec3 eyeDir;"
	"void main()"
	"{"
	"gl_Position = model * vec4(vertexPosition.xyz, 1.0f);"
	"eyeDir = normalize(eyePos - gl_Position.xyz);"
	"gl_Position = vp * (gl_Position);"
	"normalF = vertexNormal;"
	"}"

};

const char *FragmentShaderTex = 
{
	"#version 330\n"

	"out vec4 color;"

	"in vec2 texcoord;"
	"uniform sampler2D tex;"

	"void main()"
	"{"
	"color = texture(tex, texcoord);"
	"float d = length(texcoord - vec2(0.5f, 0.5f)) / 0.8f;"
	"d *= d;"
	"color = mix(color, vec4(0,0,0,1), d);"
	//"//texture(tex, texcoord);"
	"}"
};

const char *VertexShaderTex = 
{
	"#version 330\n"

	"layout(location = 0) in vec3 vertexPosition;"
	"layout(location = 1) in vec2 vertexTexcoord;"

	"out vec2 texcoord;"
	"void main()"
	"{"
	"gl_Position = vec4(vertexPosition.xyz, 1.0f);"
	"texcoord = vertexTexcoord;"
	"}"
};

