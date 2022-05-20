#include "BloomPass.h"
#include "postProcessing.h"

extern FrameBufferObject* gFBO;

BloomPass::BloomPass() : Pass(std::string("bloom"), std::string("platinum/data/shaders/postfxV.glsl"), std::string("platinum/data/shaders/postfxBlurF.glsl"), PassBuffers::Color | PassBuffers::Depth, TGE::ShapeBaseObjectType)
{
}

void BloomPass::processPass(Point2I extent) {
	// post processing
	glViewport(0, 0, extent.x, extent.y);
	glPushMatrix();
	glLoadIdentity();

	glPushMatrix();
	glLoadIdentity();

	bool result = (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

	glBindFramebuffer(GL_FRAMEBUFFER, this->finalFrameBuffer);
	GL_CheckErrors("render bloom");
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	// bind shader and texture uniform
	if (!glIsEnabled(GL_TEXTURE_2D))
		glEnable(GL_TEXTURE_2D);

	// Pass 1

	int bloomQualityLevel = TGE::Con::getIntVariable("$pref::Video::ShapeBloomQuality");

	if (bloomQualityLevel == 0) {
		if (glIsEnabled(GL_TEXTURE_2D))
			glDisable(GL_TEXTURE_2D);

		glPopMatrix();
		return;
	}

	this->shader->activate();
	this->shader->setUniformLocation("textureSampler", 0);
	this->shader->setUniformLocation("depthSampler", 1);
	this->shader->setUniformLocation("bloomDepthSampler", 2);
	glUniform1i(this->shader->getUniformLocation("compareDepth"), 1);
	glUniform1i(this->shader->getUniformLocation("bloomQuality"), bloomQualityLevel);
	glUniform1i(this->shader->getUniformLocation("passNum"), 1);
	glUniform1ui(this->shader->getUniformLocation("horizontalDir"), 1);
	glUniform1f(this->shader->getUniformLocation("offsetMultiplier"), 1.0f);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, this->colorBuffer);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gFBO->depthTextureHandle);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, this->depthBuffer);
	glUniform2f(this->shader->getUniformLocation("screenSize"), static_cast<F32>(extent.x), static_cast<F32>(extent.y));
	GL_CheckErrors("activate bloom shader");

	// send verts
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, gFBO->quadVBO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	GL_CheckErrors("send verts");
	// Draw the quad! Remember, it's 2 triangles!
	glDrawArrays(GL_TRIANGLES, 0, 6);
	GL_CheckErrors("draw quad");
	this->shader->deactivate();

	if (bloomQualityLevel > 1) {

		// Pass 2-5

		for (int i = 0; i < 2; i++) {
			glBindFramebuffer(GL_FRAMEBUFFER, this->frameBuffer);
			GL_CheckErrors("render bloom: 2");
			glClearColor(0, 0, 0, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			this->shader->activate();
			this->shader->setUniformLocation("textureSampler", 0);
			this->shader->setUniformLocation("depthSampler", 1);
			this->shader->setUniformLocation("bloomDepthSampler", 2);
			glUniform1i(this->shader->getUniformLocation("compareDepth"), 0);
			glUniform1i(this->shader->getUniformLocation("bloomQuality"), bloomQualityLevel);
			glUniform1i(this->shader->getUniformLocation("passNum"), 2+(i*2));

			glUniform1ui(this->shader->getUniformLocation("horizontalDir"), i == 0 ? 1 : 0);
			glUniform1f(this->shader->getUniformLocation("offsetMultiplier"), -1.0f);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, this->finalColorBuffer);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, gFBO->depthTextureHandle);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, this->finalDepthBuffer);
			glUniform2f(this->shader->getUniformLocation("screenSize"), static_cast<F32>(extent.x), static_cast<F32>(extent.y));
			GL_CheckErrors("activate bloom shader");

			// send verts
			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, gFBO->quadVBO);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
			GL_CheckErrors("send verts");
			// Draw the quad! Remember, it's 2 triangles!
			glDrawArrays(GL_TRIANGLES, 0, 6);
			GL_CheckErrors("draw quad");
			this->shader->deactivate();

			// Pass 3
			glBindFramebuffer(GL_FRAMEBUFFER, this->finalFrameBuffer);
			GL_CheckErrors("render bloom: 2");
			glClearColor(0, 0, 0, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			this->shader->activate();
			this->shader->setUniformLocation("textureSampler", 0);
			this->shader->setUniformLocation("depthSampler", 1);
			this->shader->setUniformLocation("bloomDepthSampler", 2);
			glUniform1i(this->shader->getUniformLocation("compareDepth"), 0);
			glUniform1i(this->shader->getUniformLocation("bloomQuality"), bloomQualityLevel);
			glUniform1i(this->shader->getUniformLocation("passNum"), 3 + (i * 2));

			glUniform1ui(this->shader->getUniformLocation("horizontalDir"), 0);
			glUniform1f(this->shader->getUniformLocation("offsetMultiplier"), 1.0f);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, this->colorBuffer);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, gFBO->depthTextureHandle);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, this->depthBuffer);
			glUniform2f(this->shader->getUniformLocation("screenSize"), static_cast<F32>(extent.x), static_cast<F32>(extent.y));
			GL_CheckErrors("activate bloom shader");

			// send verts
			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, gFBO->quadVBO);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
			GL_CheckErrors("send verts");
			// Draw the quad! Remember, it's 2 triangles!
			glDrawArrays(GL_TRIANGLES, 0, 6);
			GL_CheckErrors("draw quad");
			this->shader->deactivate();
		}
	}


	if (glIsEnabled(GL_TEXTURE_2D))
		glDisable(GL_TEXTURE_2D);

	glPopMatrix();
}