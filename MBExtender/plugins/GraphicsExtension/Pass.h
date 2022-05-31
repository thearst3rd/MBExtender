#include "GraphicsExtension.h"
#include <GLHelper/GLHelper.h>

#pragma once

class Pass {

public:
	enum PassBuffers {
		None,
		Color,
		Depth
	};
	Pass(std::string name, std::string shaderPathV, std::string shaderPathF, int flags, int renderFlags);
	~Pass();
	std::string name;
	std::string shaderPathV;
	std::string shaderPathF;
	int flags;
	int renderFlags;
	GLuint frameBuffer = 0;
	GLuint colorBuffer = 0;
	GLuint depthBuffer = 0;
	GLuint finalFrameBuffer = 0;
	GLuint finalColorBuffer = 0;
	GLuint finalDepthBuffer = 0;
	Shader* shader;

	void initShader();
	bool initBuffers(Point2I& extent);
	void unload();
	void render(Point2I& extent);
	virtual void processPass(Point2I& extent);
};