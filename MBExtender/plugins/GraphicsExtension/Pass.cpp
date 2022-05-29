#include "Pass.h"
#include "postProcessing.h"

extern std::string currentPass;

Pass::Pass(std::string name, std::string shaderPathV, std::string shaderPathF, int flags, int renderFlags) {
	this->name = name;
	this->shaderPathV = shaderPathV;
	this->shaderPathF = shaderPathF;
	this->flags = flags;
	this->renderFlags = renderFlags;
}

Pass::~Pass() {
	this->unload();
}

void Pass::initShader() {
	this->shader = new Shader(this->shaderPathV, this->shaderPathF);
}

bool Pass::initBuffers(Point2I extent) {
	glGenFramebuffers(1, &this->frameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, this->frameBuffer);

	glGenTextures(1, &this->colorBuffer);
	glBindTexture(GL_TEXTURE_2D, this->colorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, extent.x, extent.y, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->colorBuffer, 0);


	glGenTextures(1, &this->depthBuffer);
	glBindTexture(GL_TEXTURE_2D, this->depthBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, extent.x, extent.y, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, this->depthBuffer, 0);


	glBindTexture(GL_TEXTURE_2D, 0);


	glGenFramebuffers(1, &this->finalFrameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, this->finalFrameBuffer);

	if ((this->flags & PassBuffers::Color) != 0) {
		glGenTextures(1, &this->finalColorBuffer);
		glBindTexture(GL_TEXTURE_2D, this->finalColorBuffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, extent.x, extent.y, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->finalColorBuffer, 0);
	}

	if ((this->flags & PassBuffers::Depth) != 0) {
		glGenTextures(1, &this->finalDepthBuffer);
		glBindTexture(GL_TEXTURE_2D, this->finalDepthBuffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, extent.x, extent.y, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, this->finalDepthBuffer, 0);
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	bool result = (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (!result) {
		return false;
	}
	return true;

}

void Pass::unload() {
	if (this->shader != NULL)
		delete this->shader;

	glDeleteTextures(1, &this->colorBuffer);
	this->colorBuffer = 0;
	glDeleteTextures(1, &this->depthBuffer);
	this->depthBuffer = 0;

	if ((this->flags & PassBuffers::Color) != 0) {
		glDeleteTextures(1, &this->finalColorBuffer);
		this->finalColorBuffer = 0;
	}
	if ((this->flags & PassBuffers::Depth) != 0) {
		glDeleteTextures(1, &this->finalDepthBuffer);
		this->finalDepthBuffer = 0;
	}

	glDeleteFramebuffers(1, &this->frameBuffer);
	this->frameBuffer = 0;

	glDeleteFramebuffers(1, &this->finalFrameBuffer);
	this->finalFrameBuffer = 0;
}

void Pass::render(Point2I extent) {
	currentPass = this->name;
	glBindFramebuffer(GL_FRAMEBUFFER, this->frameBuffer);
	GL_CheckErrors("activate bloombuffer");
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	this->prerender();
	renderGame(renderFlags);
	this->postrender();
	GL_CheckErrors("render game");
	this->processPass(extent);
}

void Pass::processPass(Point2I extent) {

}

void Pass::prerender() {

}

void Pass::postrender() {

}