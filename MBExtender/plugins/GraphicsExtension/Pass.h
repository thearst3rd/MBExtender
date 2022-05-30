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
	GLuint frameBuffer;
	GLuint colorBuffer;
	GLuint depthBuffer;
	GLuint finalFrameBuffer;
	GLuint finalColorBuffer;
	GLuint finalDepthBuffer;
	Shader* shader;

	void initShader();
	bool initBuffers(Point2I extent);
	void unload();
	void render(Point2I extent);
	virtual void processPass(Point2I extent);
};