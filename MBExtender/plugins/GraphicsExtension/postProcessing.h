#include "GraphicsExtension.h"
#include <GLHelper/GLHelper.h>
#pragma once

struct FrameBufferObject {
	GLuint colorTextureHandle = 0;
	GLuint depthTextureHandle = 0;
	GLuint multisampleColorTextureHandle = 0;
	GLuint multisampleDepthTextureHandle = 0;
	GLuint bufferHandle = 0;
	GLuint multisampleBufferHandle = 0;
	GLuint depthBufferHandle = 0;
	GLuint finalRenderBufferHandle = 0;
	GLuint finalRenderColorTextureHandle = 0;
	GLuint screenSizeLocation = 0;
	bool setUpProperly = false;
	bool texturesLoaded = false;

	GLuint quadVBO = 0;

	Shader* shader = nullptr;
};


void setUpPostProcessing();
bool initNonMultisampleFrameBuffer(Point2I extent);
bool initMultisampleFrameBuffer(Point2I extent);
void initDepthBuffer(Point2I extent);
void initQuadBuffer();
void initFBOShader();
void renderGame(U32 mask);
bool hasMultisampleBuffer();