#include <iostream>
#include <SDL.h>
#include <GL\glew.h>
#include <glm.hpp>
#include "gtc\matrix_transform.hpp"
#include "Shaders.h"
#include "mesh.h"
#include <windows.h>

#include "imgui.h"
#include "imgui_impl_sdl_gl3.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

#define PI 3.14159265f
//#define CUBE_HAIR


SDL_Window *window;
bool InitWindow()
{
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
		return false;

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
	window = SDL_CreateWindow("Hair", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
							  SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

	SDL_GLContext glContext = SDL_GL_CreateContext(window);

	if (SDL_GL_SetSwapInterval(1) == -1)
		SDL_GL_SetSwapInterval(0);

	glewExperimental = true;
	GLenum err = glewInit();

	ImGui_ImplSdlGL3_Init(window);

	glCullFace(GL_BACK);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	return true;
}

struct VoxelSpace
{
	float *densities;
	glm::vec3 *velocities;
	int sizeX;
	int sizeY;
	int sizeZ;

	float measureX;
	float measureY;
	float measureZ;
};

void GetVertexCoordinates(VoxelSpace *voxelSpace, glm::vec3 pos, int ID, int *x, int *y, int *z, glm::vec3 *position)
{
	pos.x /= voxelSpace->measureX;
	pos.y /= voxelSpace->measureY;
	pos.z /= voxelSpace->measureZ;

	switch (ID)
	{
		case 0:
			*x = (int)glm::floor(pos.x);
			*y = (int)glm::floor(pos.y);
			*z = (int)glm::floor(pos.z);
			break;
		case 1:
			*x = (int)glm::floor(pos.x);
			*y = (int)glm::floor(pos.y);
			*z = (int)glm::floor(pos.z) + 1;
			break;
		case 2:
			*x = (int)glm::floor(pos.x);
			*y = (int)glm::floor(pos.y) + 1;
			*z = (int)glm::floor(pos.z);
			break;
		case 3:
			*x = (int)glm::floor(pos.x);
			*y = (int)glm::floor(pos.y) + 1;
			*z = (int)glm::floor(pos.z) + 1;
			break;
		case 4:
			*x = (int)glm::floor(pos.x) + 1;
			*y = (int)glm::floor(pos.y);
			*z = (int)glm::floor(pos.z);
			break;
		case 5:
			*x = (int)glm::floor(pos.x) + 1;
			*y = (int)glm::floor(pos.y);
			*z = (int)glm::floor(pos.z) + 1;
			break;
		case 6:
			*x = (int)glm::floor(pos.x) + 1;
			*y = (int)glm::floor(pos.y) + 1;
			*z = (int)glm::floor(pos.z);
			break;
		case 7:
			*x = (int)glm::floor(pos.x) + 1;
			*y = (int)glm::floor(pos.y) + 1;
			*z = (int)glm::floor(pos.z) + 1;
			break;
	}

	position->x = *x * voxelSpace->measureX;
	position->y = *y * voxelSpace->measureY;
	position->z = *z * voxelSpace->measureZ;
}

GLuint CreateProgram(const GLchar *vertexShaderString, const GLchar *fragmentShaderString)
{
	GLuint program = glCreateProgram();

	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	const GLchar * vertexShaderSource = vertexShaderString;
	glShaderSource(vertexShader, 1, (const GLchar **)&vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	int compileStatus;
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &compileStatus);
	if (compileStatus != GL_TRUE)
		return 0;
	glAttachShader(program, vertexShader);

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	std::string fragmentString = fragmentShaderString;
	const GLchar* fragmentShaderSource = fragmentString.c_str();
	glShaderSource(fragmentShader, 1, (const GLchar **)&fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &compileStatus);
	if (compileStatus != GL_TRUE)
		return 0;
	glAttachShader(program, fragmentShader);

	glLinkProgram(program);

	return program;
}

int main(int argc, char **argv)
{
	SDL_Event e;
	bool isRunning = true;
	Uint32 time = SDL_GetTicks();
	bool isPressed = false;

	if(!InitWindow())
		return 1;


	struct Particle
	{
		glm::vec3 position;
		glm::vec3 oldPosition;
		glm::vec3 velocity;
		glm::vec3 rigidVelocity;
		int connectionTop;
	};

//#define HEAD
#define HAIR_DISTANCE 0.001f
#ifdef HEAD
	#define HAIR_COUNT 1000
#else
	#define HAIR_COUNT_X 50
	#define HAIR_COUNT_Z 50
	#define HAIR_COUNT (HAIR_COUNT_X * HAIR_COUNT_Z)
#endif
#define HAIR_LENGTH 10
	struct Hair
	{
		Particle particles[HAIR_LENGTH];
	};

#define HAIR_PART_LENGTH 0.05f
	Hair *hair = new Hair[HAIR_COUNT];
	glm::vec4 *translations = new glm::vec4[2 * HAIR_COUNT * HAIR_LENGTH];

	float headRadius = 0.5f;
	glm::vec3 headCenter = glm::vec3(0, 0, 0);

	srand(NULL);
	auto Random = []() -> float
	{
		return (rand() % 1000) / 1000.0f;
	};

	auto InitHair = [Random, headRadius](Hair *hair)
	{
		for (int i = 0; i < HAIR_COUNT; ++i)
		{
			float azimuth = Random() * PI * 2.0f;
			float polar = glm::acos(Random() * 0.6f + 0.4f);
			glm::vec3 spherePos = glm::vec3(glm::sin(polar) * glm::cos(azimuth), glm::cos(polar), glm::sin(polar) * glm::sin(azimuth));
			spherePos *= headRadius;
			for (int h = 0; h < HAIR_LENGTH; ++h)
			{
#ifdef HEAD
				hair[i].particles[h].position = spherePos + glm::vec3(0,0,h * HAIR_DISTANCE);
#else
				hair[i].particles[h].position = glm::vec3((i % HAIR_COUNT_X) * HAIR_DISTANCE, 
														  0, 
														  (i / HAIR_COUNT_X) * HAIR_DISTANCE);// + (h % HAIR_LENGTH) * HAIR_PART_LENGTH);
				hair[i].particles[h].position.x -= HAIR_COUNT_X * HAIR_DISTANCE / 2.0f;
				hair[i].particles[h].position.z -= HAIR_COUNT_Z * HAIR_DISTANCE / 2.0f;
#endif
				hair[i].particles[h].velocity = glm::vec3(0, 0, 0);
				hair[i].particles[h].rigidVelocity = glm::vec3(0, 0, 0);
				hair[i].particles[h].connectionTop = h == 0 ? -1 : h - 1;
			}
		}
	};

	auto UpdateTranslationBuffer = [](Hair *hair, glm::vec4 *translations)
	{
		for (int i = 0; i < HAIR_COUNT; ++i)
		{
			for (int h = 0; h < HAIR_LENGTH; ++h)
			{
				translations[2 * i * HAIR_LENGTH + 2 * h] = glm::vec4(hair[i].particles[h].position, 1);
				int previous = max(h - 1, 0);
				int next = min(h + 1, HAIR_LENGTH - 1);
				glm::vec3 tangent = glm::normalize(hair[i].particles[previous].position - hair[i].particles[next].position);
				translations[2 * i * HAIR_LENGTH + 2 * h + 1] = glm::vec4(tangent, 0);
			}
		}
	};

	InitHair(hair);
	UpdateTranslationBuffer(hair, translations);

	GLuint texVao;

	{
		glGenVertexArrays(1, &texVao);
		glBindVertexArray(texVao);

		GLuint texVBO, texEBO;
		glGenBuffers(1, &texVBO);
		glBindBuffer(GL_ARRAY_BUFFER, texVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(screenQuadVertices), screenQuadVertices, GL_STATIC_DRAW);

		glGenBuffers(1, &texEBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, texEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(screenQuadIndices), screenQuadIndices, GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, 0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void *)(sizeof(float) * 3));
	}

	GLuint vao, vbo;
	const int indexCount = HAIR_COUNT * (HAIR_LENGTH - 1) * 2;
	{
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		GLuint ebo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * HAIR_COUNT * HAIR_LENGTH * 2, translations, GL_DYNAMIC_DRAW);

		GLushort indices[indexCount];
		int offset = 0;
		for (int i = 0; i < HAIR_COUNT * (HAIR_LENGTH - 1); ++i)
		{
			indices[i * 2] = i + offset;
			indices[i * 2 + 1] = i + 1 + offset;
			if ((i + 1) % (HAIR_LENGTH - 1) == 0)
				++offset;
		}

		glGenBuffers(1, &ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, 0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void *)(sizeof(float) * 4));
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);

		glBindVertexArray(0);
	}

	GLuint renderTargetMS;
	GLuint renderTarget;
	{
		GLuint renderTargetTexMS, renderTargetDepthMS, renderTargetTex;
		glGenTextures(1, &renderTargetTexMS);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, renderTargetTexMS);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, SCREEN_WIDTH, SCREEN_HEIGHT, GL_TRUE);

		glGenRenderbuffers(1, &renderTargetDepthMS);
		glBindRenderbuffer(GL_RENDERBUFFER, renderTargetDepthMS);
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT16, SCREEN_WIDTH, SCREEN_HEIGHT);

		glGenFramebuffers(1, &renderTargetMS);
		glBindFramebuffer(GL_FRAMEBUFFER, renderTargetMS);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, renderTargetTexMS, 0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderTargetDepthMS);
		glEnable(GL_MULTISAMPLE);

		glGenTextures(1, &renderTargetTex);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, renderTargetTex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);

		glGenFramebuffers(1, &renderTarget);
		glBindFramebuffer(GL_FRAMEBUFFER, renderTarget);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderTargetTex, 0);
	}

	GLuint headVao;
	{

		glGenVertexArrays(1, &headVao);
		glBindVertexArray(headVao);
		GLuint headVbo, headEbo;
	#define PI 3.1415f
	
		glm::vec3 head[200 * 2];
		for (int i = 0; i < 20; ++i)
		{
			float azimuth = i * PI * 2.0f / 20.0f;
			for (int j = 0; j < 10; ++j)
			{
				float polar = j * PI / 10.0f;
				glm::vec3 pos = glm::vec3(glm::sin(polar) * glm::cos(azimuth), glm::cos(polar), glm::sin(polar) * glm::sin(azimuth));
				glm::vec3 normal = glm::normalize(pos);
				int vertexID = i * 10 + j;

				head[vertexID * 2] = pos * headRadius;
				head[vertexID * 2 + 1] = normal;
			}
		}

		GLushort headIndices[20 * 10 * 6];
		for (int i = 0; i < 20; ++i)
		{
			for (int j = 0; j < 10; ++j)
			{
				int vertexID1 = i * 10 + j;
				int vertexID2 = i * 10 + j + 1;
				int vertexID3 = ((i + 1) % 20) * 10 + j;
				int vertexID4 = ((i + 1) % 20) * 10 + j + 1;
				int indexID = i * 10 + j;
				headIndices[indexID * 6] = vertexID3;
				headIndices[indexID * 6 + 1] = vertexID2;
				headIndices[indexID * 6 + 2] = vertexID1;
				headIndices[indexID * 6 + 3] = vertexID4;
				headIndices[indexID * 6 + 4] = vertexID2;
				headIndices[indexID * 6 + 5] = vertexID3;
			}
		}



		glGenBuffers(1, &headVbo);
		glBindBuffer(GL_ARRAY_BUFFER, headVbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * 200 * 2, head, GL_STATIC_DRAW);

		glGenBuffers(1, &headEbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, headEbo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * 20 * 10 * 6, headIndices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, 0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void *)(sizeof(float) * 3));
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glBindVertexArray(0);
	}

	GLuint hairProgram = CreateProgram(VertexShaderMesh, FragmentShaderHair);
	GLuint headProgram = CreateProgram(VertexShaderMesh, FragmentShaderHead);
	GLuint texProgram = CreateProgram(VertexShaderTex, FragmentShaderTex);

	glUseProgram(texProgram);
	GLuint texLocation = glGetUniformLocation(texProgram, "tex");

	glUseProgram(hairProgram);
	GLuint modelMatrixHair = glGetUniformLocation(hairProgram, "model");
	GLuint vpMatrixHair = glGetUniformLocation(hairProgram, "vp");
	GLuint eyePosHair = glGetUniformLocation(hairProgram, "eyePos");

	glUseProgram(headProgram);
	GLuint modelMatrixHead = glGetUniformLocation(headProgram, "model");
	GLuint vpMatrixHead = glGetUniformLocation(headProgram, "vp");
	GLuint eyePosHead = glGetUniformLocation(headProgram, "eyePos");

	auto SetEyePos = [eyePosHair, eyePosHead, hairProgram, headProgram](glm::vec3 pos)
	{
		glUseProgram(hairProgram);
		glUniform3f(eyePosHair, pos.x, pos.y, pos.z);
		glUseProgram(headProgram);
		glUniform3f(eyePosHead, pos.x, pos.y, pos.z);
	};


	struct Camera
	{
		float polar = 1.5f;
		float azimuth = 10;
		float distance = 5.0f;
	};
	Camera camera;

	auto GetVPFromCamera = [SetEyePos](Camera *camera) -> glm::mat4
	{
		glm::vec3 newPosition = glm::vec3(glm::sin(camera->polar) * glm::cos(camera->azimuth),
										  glm::cos(camera->polar),
										  glm::sin(camera->polar) * glm::sin(camera->azimuth));
		newPosition *= camera->distance;
		SetEyePos(newPosition);
		glm::mat4 model = glm::perspective(45.0f, 800.0f / 600.0f, 0.01f, 100.0f);
		glm::vec3 up = camera->polar < 0.01f? 
			glm::vec3(-glm::cos(camera->azimuth), 0, -glm::sin(camera->azimuth)) : 
			glm::vec3(0,1,0);
		model = model * glm::lookAt(newPosition, glm::vec3(0, 0, 0), up);
		return model;
	};


	glm::mat4 vp = GetVPFromCamera(&camera);
	glUseProgram(hairProgram);
	glUniformMatrix4fv(vpMatrixHair, 1, GL_FALSE, &vp[0][0]);
	glUseProgram(headProgram);
	glUniformMatrix4fv(vpMatrixHead, 1, GL_FALSE, &vp[0][0]);

	auto ResetVoxelSpace = [](VoxelSpace *voxelSpace)
	{
		int sizeX = voxelSpace->sizeX;
		int sizeY = voxelSpace->sizeY;
		int sizeZ = voxelSpace->sizeZ;
		for (int z = 0; z < sizeZ; ++z)
		{
			for (int y = 0; y < sizeY; ++y)
			{
				for (int x = 0; x < sizeX; ++x)
				{
					voxelSpace->densities[z * sizeY * sizeX + y * sizeX + x] = 0.0f;
					voxelSpace->velocities[z * sizeY * sizeX + y * sizeX + x] = glm::vec3(0, 0, 0);
				}
			}
		}
	};

	auto GetVoxelSpace = [ResetVoxelSpace](int sizeX, int sizeY, int sizeZ, float measureX, float measureY, float measureZ)
		-> VoxelSpace
	{
		VoxelSpace voxelSpace;
		voxelSpace.densities = new float[sizeX * sizeY * sizeZ];
		voxelSpace.velocities = new glm::vec3[sizeX * sizeY * sizeZ];
		voxelSpace.sizeX = sizeX;
		voxelSpace.sizeY = sizeY;
		voxelSpace.sizeZ = sizeZ;
		voxelSpace.measureX = measureX;
		voxelSpace.measureY = measureY;
		voxelSpace.measureZ = measureZ;
		ResetVoxelSpace(&voxelSpace);
		return voxelSpace;
	};

	auto UpdateVoxelVelocity = [](VoxelSpace *voxelSpace, glm::vec3 position, glm::vec3 velocity)
	{
		int sX = voxelSpace->sizeX;
		int sY = voxelSpace->sizeY;
		int sZ = voxelSpace->sizeZ;
		for (int v = 0; v < 8; ++v)
		{
			int vX, vY, vZ;
			glm::vec3 vertexPos;
			GetVertexCoordinates(voxelSpace, position, v, &vX, &vY, &vZ, &vertexPos);
			float ax = (1 - glm::abs(position.x - vertexPos.x)/voxelSpace->measureX);
			float ay = (1 - glm::abs(position.y - vertexPos.y)/voxelSpace->measureY);
			float az = (1 - glm::abs(position.z - vertexPos.z)/voxelSpace->measureZ);
			float trilinearWeight = ax * ay * az;
			vX = vX + sX / 2;
			vY = vY + sY / 2;
			vZ = vZ + sZ / 2;
			assert(vX < sX && vX >= 0 && vY < sY && vY >= 0 && vZ < sZ && vZ >= 0);
			voxelSpace->densities[vZ * sY * sX + vY * sX + vX] += trilinearWeight;
			voxelSpace->velocities[vZ * sY * sX + vY * sX + vX] += velocity * trilinearWeight;
		}
	};

	auto GetVoxelData = [](VoxelSpace *voxelSpace, glm::vec3 pos, glm::vec3 *avgVelocity, glm::vec3 *densityGradient, glm::vec3 *normalizedPosition)
	{
		avgVelocity->x = 0;
		avgVelocity->y = 0;
		avgVelocity->z = 0;
		int sX = voxelSpace->sizeX;
		int sY = voxelSpace->sizeY;
		int sZ = voxelSpace->sizeZ;
		for (int v = 0; v < 8; ++v)
		{
			int vX, vY, vZ;
			glm::vec3 vertexPos;
			GetVertexCoordinates(voxelSpace, pos, v, &vX, &vY, &vZ, &vertexPos);
			float ax = (1 - glm::abs(pos.x - vertexPos.x)/voxelSpace->measureX);
			float ay = (1 - glm::abs(pos.y - vertexPos.y)/voxelSpace->measureY);
			float az = (1 - glm::abs(pos.z - vertexPos.z)/voxelSpace->measureZ);
			float trilinearWeight = ax * ay * az;
			*normalizedPosition = glm::vec3(ax,ay,az);
			vX = vX + sX / 2;
			vY = vY + sY / 2;
			vZ = vZ + sZ / 2;
			assert(vX < sX && vX >= 0 && vY < sY && vY >= 0 && vZ < sZ && vZ >= 0);
			if (voxelSpace->densities[vZ * sY * sX + vY * sX + vX] > 0.0f)
			{
				if (pos.x >= vertexPos.x)
					densityGradient->x += ay * az * voxelSpace->densities[vZ * sY * sX + vY * sX + vX];
				else
					densityGradient->x -= ay * az * voxelSpace->densities[vZ * sY * sX + vY * sX + vX];
				if (pos.y >= vertexPos.y)
					densityGradient->y += ax * az * voxelSpace->densities[vZ * sY * sX + vY * sX + vX];
				else
					densityGradient->y -= ax * az * voxelSpace->densities[vZ * sY * sX + vY * sX + vX];
				if (pos.z >= vertexPos.z)
					densityGradient->z += ax * ay * voxelSpace->densities[vZ * sY * sX + vY * sX + vX];
				else
					densityGradient->z -= ax * ay * voxelSpace->densities[vZ * sY * sX + vY * sX + vX];

				*avgVelocity += trilinearWeight * voxelSpace->velocities[vZ * sY * sX + vY * sX + vX] /
					voxelSpace->densities[vZ * sY * sX + vY * sX + vX];
			}
		}
	};

	VoxelSpace voxelSpace = GetVoxelSpace(150,150,150,0.02f, 0.02f, 0.02f);

	int oldMouseX;
	int oldMouseY;
	float timer = 0;
	bool dirtyWheel = false;

	float damping = 1.0f;//.1f;
	float friction = 0.001f;
	float repulsion = 3.0f;
	float windStrength = 0.0f;
	float *option = &repulsion;
	float granularity = 0.1f;
	while (isRunning)
	{
		dirtyWheel = false;
		while (SDL_PollEvent(&e) != 0)
		{
			ImGui_ImplSdlGL3_ProcessEvent(&e);

			const Uint8 *state = SDL_GetKeyboardState(NULL);
			switch (e.type)
			{
				case SDL_QUIT:
					isRunning = false;
					break;
				case SDL_MOUSEWHEEL:
					camera.distance -= e.wheel.y * 0.5f;
					if (camera.distance < .5f)
					{
						camera.distance = 0.5f;
					}
					dirtyWheel = true;
					break;
				case SDL_KEYDOWN:
					if (state[SDL_SCANCODE_R])
					{
						isPressed = true;
						InitHair(hair);
						UpdateTranslationBuffer(hair, translations);
					}
					if (state[SDL_SCANCODE_F1])
					{
						option = &damping;
						granularity = 1.0f;
					}
					if (state[SDL_SCANCODE_F2])
					{
						option = &repulsion;
						granularity = 0.1f;
					}
					if (state[SDL_SCANCODE_F3])
					{
						option = &friction;
						granularity = 0.01f;
					}
					if (state[SDL_SCANCODE_ESCAPE])
					{
						isRunning = false;
					}
					break;
				case SDL_KEYUP:
					isPressed = false;
					break;
			}
			
		}
		ImGui_ImplSdlGL3_NewFrame();
		bool sliderUsed = false;
		ImGui::SetNextWindowSize(ImVec2(100, 100), ImGuiSetCond_Appearing);
		ImGui::Begin("Hair control", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
		ImGui::SetWindowSize(ImVec2(400,100), ImGuiSetCond_Always);
		sliderUsed = ImGui::SliderFloat("s_repulsion", &repulsion, 0, 12.0f);
		sliderUsed = ImGui::SliderFloat("s_damp", &damping, 0, 50.0f) || sliderUsed;
		sliderUsed = ImGui::SliderFloat("s_wind", &windStrength, 0, 50.0f) || sliderUsed;
		ImGui::End();

		Uint32 newTime = SDL_GetTicks();
		float dt = min(0.016f, (newTime - time) / 1000.0f);
		timer += dt;
		time = newTime;
		int mouseX, mouseY;
		if (!sliderUsed && ((SDL_GetMouseState(&mouseX, &mouseY) & SDL_BUTTON(SDL_BUTTON_LEFT)) || dirtyWheel))
		{
			if (oldMouseX >= 0)
			{
				camera.azimuth += (mouseX - oldMouseX) * 0.05f;
				camera.polar -= (mouseY - oldMouseY) * 0.05f;
				if(camera.polar < 0)
					camera.polar = 0;
				if(camera.polar > 3.1415f)
					camera.polar = 3.1415f;
			}
			glm::mat4 vp = GetVPFromCamera(&camera);
			glUseProgram(hairProgram);
			glUniformMatrix4fv(vpMatrixHair, 1, GL_FALSE, &vp[0][0]);
			glUseProgram(headProgram);
			glUniformMatrix4fv(vpMatrixHead, 1, GL_FALSE, &vp[0][0]);
			if (!dirtyWheel)
			{
				oldMouseX = mouseX;
				oldMouseY = mouseY;
			}

		}
		else
		{
			oldMouseX = -1;
		}
		ResetVoxelSpace(&voxelSpace);
		for (int i = 0; i < HAIR_COUNT; ++i)
		{
			for (int h = 0; h < HAIR_LENGTH; ++h)
			{
				glm::vec3 pos = hair[i].particles[h].position;
				int top = hair[i].particles[h].connectionTop;
				if (top >= 0)
				{
					UpdateVoxelVelocity(&voxelSpace, pos, hair[i].particles[h].velocity);
				}
			}
		}

		for (int i = 0; i < HAIR_COUNT; ++i)
		{
			for (int h = 0; h < HAIR_LENGTH; ++h)
			{
				Particle *particle = &hair[i].particles[h];
				int top = particle->connectionTop;

				glm::vec3 pos = particle->position;
				particle->oldPosition = pos;
				if (top >= 0)
				{
					glm::vec3 avgVelocity = glm::vec3(0, 0, 0);
					glm::vec3 densityGradient = glm::vec3(0, 0, 0);
					glm::vec3 normalizedPosition = glm::vec3(0,0,0);
					GetVoxelData(&voxelSpace, pos, &avgVelocity, &densityGradient, &normalizedPosition);

					if(glm::length(densityGradient) > 0)
						densityGradient /= glm::length(densityGradient);
					
					particle->velocity = (1 - friction) * particle->velocity + friction * avgVelocity + glm::vec3(0, -9.8f, 0) * dt;
					particle->velocity += repulsion * densityGradient * dt;		

					float x = particle->position.x * 4.0f * PI + timer * 15.0f;
					float y = particle->position.y * 3.5f * PI + timer * 15.0f;

					float wind = glm::cos(x) + 
						0.3333f * glm::cos(3 * x) + 
						0.16666f * glm::cos(6 * y) + 
						0.5f * glm::sin(2 * y) + 
						0.25f * glm::sin(4 * x);
					wind = glm::clamp(wind, 0.0f, 1.0f);
					particle->velocity.z += windStrength * wind * dt * glm::inversesqrt(2.0f);
					particle->velocity.x += windStrength * wind * dt * glm::inversesqrt(2.0f);

					particle->velocity -= particle->velocity * damping * dt;

					glm::vec3 newPosition = particle->position + particle->velocity * dt;
						
					glm::vec3 topParticleVelocityUpdate = glm::vec3(0);
					bool constrainsSatisfied = true;
					float coef = 1.0f;
					int counter = 0;
					do
					{
						constrainsSatisfied = true;
#ifdef HEAD
						glm::vec3 dirToHead = headCenter - newPosition;
						if (glm::length(dirToHead) < headRadius)
						{
							glm::vec3 correctionHead = dirToHead - glm::normalize(dirToHead) * (headRadius);
							newPosition += correctionHead;
							constrainsSatisfied = false;
						}
#endif

						glm::vec3 dir = hair[i].particles[top].position - newPosition;
						glm::vec3 correction = dir - glm::normalize(dir) * HAIR_PART_LENGTH;
						if (glm::length(dir) > HAIR_PART_LENGTH)
						{
							newPosition += correction * coef;
							topParticleVelocityUpdate = 0.9f * coef * correction / dt;
						}
						++counter;
					}
					while(!constrainsSatisfied && counter < 10);


					particle->velocity = (newPosition - particle->position) / dt;
					particle->position = newPosition;

#ifdef HEAD
					glm::vec3 part1 = h > 1 ? hair[i].particles[top - 1].position : headCenter;
					glm::vec3 part2 = hair[i].particles[top].position;
					glm::vec3 normal = glm::normalize(part2 - part1);
					glm::vec3 idealPosition = part2 + normal * HAIR_PART_LENGTH;
					glm::vec3 rigidForce = 700.0f * (idealPosition - particle->position);
					particle->velocity += rigidForce * dt;
#endif

					hair[i].particles[top].velocity -= topParticleVelocityUpdate;
				}
			}
		}

		UpdateTranslationBuffer(hair, translations);

		glBindFramebuffer(GL_FRAMEBUFFER, renderTargetMS);
		glClearColor(1,1,1,1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(hairProgram);
		glm::mat4 model = glm::mat4(1.0f);//glm::scale(glm::mat4(1.0f), glm::vec3(0.2f, 0.2f, 0.2f));
		glUniformMatrix4fv(modelMatrixHair, 1, GL_FALSE, &model[0][0]);

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * HAIR_COUNT * HAIR_LENGTH * 2, translations, GL_DYNAMIC_DRAW);

		glBindVertexArray(vao);

		glDrawElements(GL_LINES, indexCount, GL_UNSIGNED_SHORT, 0);

		glUseProgram(headProgram);
		glm::mat4 modelHead = glm::translate(glm::mat4(1), headCenter);
		glUniformMatrix4fv(modelMatrixHead, 1, GL_FALSE, &modelHead[0][0]);
		glBindVertexArray(headVao);

		//glDrawElements(GL_TRIANGLES, 20 * 10 * 6, GL_UNSIGNED_SHORT, 0);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, renderTargetMS);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, renderTarget);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBlitFramebuffer(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, GL_COLOR_BUFFER_BIT, GL_NEAREST);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(texProgram);
		glUniform1i(texLocation, 1);
		glBindVertexArray(texVao);

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

		ImGui::Render();

		SDL_GL_SwapWindow(window);

	}

	return 0;
}

//http://matthias-mueller-fischer.ch/publications/FTLHairFur.pdf
//http://graphics.pixar.com/library/Hair/paper.pdf
//https://www.cs.drexel.edu/~david/Classes/CS586/Papers/p271-kajiya.pdf