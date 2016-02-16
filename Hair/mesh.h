#pragma once
#include <GL\glew.h>


float screenQuadVertices[] = {
	-1, -1, 0, 0, 0,
	1, -1, 1, 1, 0,
	1, 1, 0, 1,1,
	-1, 1, 0, 0, 1,
};

GLushort screenQuadIndices[] = {
	0, 1, 2,
	2, 3, 0,
};