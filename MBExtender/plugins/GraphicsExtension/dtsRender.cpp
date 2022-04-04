//-----------------------------------------------------------------------------
// Copyright (c) 2017, The Platinum Team
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#include <GLHelper/GLHelper.h>
#include <map>
#include <queue>
#include <vector>
#include <MBExtender/MBExtender.h>
#include "GraphicsExtension.h"
#include <MathLib/MathLib.h>
#include "DTSRenderer.h"

#include <TorqueLib/game/fx/particleEngine.h>
#include <TorqueLib/game/gameConnection.h>
#include <TorqueLib/game/marble/marble.h>
#include <TorqueLib/platform/platformVideo.h>
#include <TorqueLib/ts/tsShape.h>
#include <TorqueLib/ts/tsShapeInstance.h>
#include <TorqueLib/TypeInfo.h>
#include <TorqueLib/core/stringTable.h>
#include <TorqueLib/terrain/sky.h>
#include <TorqueLib/game/tsStatic.h>
#include <unordered_set>

#ifdef _WIN32
#include <Shlwapi.h>
#define strcasestr StrStrI
#else
#include <strings.h>
#endif

MBX_MODULE(DtsExtension);


std::unordered_map<SimObjectId, std::unordered_map<TGE::TSMesh*, DTSRenderer *>> gDtsRenderers;
TGE::ShapeBase *gCurrentRenderingShape = NULL;
TGE::TSShapeInstance *gDtsCurrentRenderingShapeInstance = NULL;
TGE::TSShapeInstance::MeshObjectInstance *gDtsCurrentRenderingObjectInstance = NULL;
std::unordered_set<TGE::TSMesh*> gMeshesToDraw;
std::unordered_set<TGE::TSMesh*> gMeshesChecked;

bool gNoShaders = false;

extern std::string currentPass;
extern std::vector<std::string> glowTextures;

class DistanceComparer {
public:
	bool operator()(TGE::Marble *m1, TGE::Marble *m2) {
		TGE::SceneObject *control = static_cast<TGE::SceneObject *>(static_cast<TGE::GameConnection *>(TGE::mServerConnection)->getControlObject());
		Point3F pos = control->getTransform().getPosition();

		Point3F m1Pos = m1->getTransform().getPosition();
		Point3F m2Pos = m2->getTransform().getPosition();

		//If m1 is closer
		return (m1Pos - pos).lenSquared() > (m2Pos - pos).lenSquared();
	}
};

DTSRenderer *createDTSRenderer(TGE::ShapeBase *shape, TGE::TSMesh* mesh) {
	DTSRenderer *renderer = new DTSRenderer();

	auto found = gDtsRenderers.find(shape->getId());
	if (found == gDtsRenderers.end()) {
		//Don't have a renderer, create one (adds to gMarbleRenderers) and init its mesh
		std::unordered_map<TGE::TSMesh*, DTSRenderer*> innermap = std::unordered_map<TGE::TSMesh*, DTSRenderer*>();
		innermap[mesh] = renderer;
		gDtsRenderers[shape->getId()] = innermap;
	}
	else {
		//Use the one we have assigned
		found->second[mesh] = renderer;
	}
	renderer->loadShader(shape);
	return renderer;
}

MBX_OVERRIDE_MEMBERFN(void, TGE::Sky::renderObject, (TGE::Sky* thisptr, TGE::SceneState* state, TGE::SceneRenderImage* renderImage), originalSkyRender) {
	if (currentPass == "fwd")
		originalSkyRender(thisptr, state, renderImage);
}

MBX_OVERRIDE_MEMBERFN(void, TGE::TSStatic::renderObject, (TGE::TSStatic* thisptr, TGE::SceneState* state, TGE::SceneRenderImage* renderImage), originalTSStaticRender) {
	if (currentPass == "fwd")
		originalTSStaticRender(thisptr, state, renderImage);
}

MBX_OVERRIDE_MEMBERFN(void, TGE::ShapeBase::renderMountedImage, (TGE::ShapeBase* thisptr, TGE::SceneState* state, TGE::SceneRenderImage* renderImage), originalRenderMImage) {
	if (currentPass == "fwd")
		originalRenderMImage(thisptr, state, renderImage);
}



MBX_OVERRIDE_MEMBERFN(void, TGE::ShapeBase::renderObject, (TGE::ShapeBase* thisptr, TGE::SceneState* state, TGE::SceneRenderImage* image), originalShapeBaseRender) {
	TGE::GameBaseData* datablock = thisptr->getDataBlock();
	if (!datablock)
		return;
	const char* shapeFile = datablock->getDataField(TGE::StringTable->insert("shapeFile", false), NULL);
	if (currentPass == "bloom") {
		gNoShaders = strstr(shapeFile, "Marble") != NULL || strstr(shapeFile, "marble") != NULL || strstr(shapeFile, "tornado") != NULL;
		if (strstr(shapeFile, "images/megamarble.dts") != NULL)
			gNoShaders = false;
	}
	else {
		gNoShaders = true;
	}

	F32 fadeVal;
	if (currentPass != "fwd") {
		fadeVal = thisptr->getFadeVal();
		thisptr->setFadeVal(1);
	}
	
	originalShapeBaseRender(thisptr, state, image);

	if (currentPass != "fwd") {
		thisptr->setFadeVal(fadeVal);
	}
}

MBX_OVERRIDE_MEMBERFN(void, TGE::ShapeBase::renderImage, (TGE::ShapeBase *thisptr, TGE::SceneState *state, TGE::SceneRenderImage *image), originalRenderImage) {
	// If we are cloaked just do an original render.
	F32 cloakLevel;
	if (currentPass != "fwd") {
		cloakLevel = thisptr->getCloakLevel();
		thisptr->setCloakLevel(0);
	}

	if (thisptr->getCloakLevel() > 0) {
		originalRenderImage(thisptr, state, image);
		return;
	}

	gCurrentRenderingShape = static_cast<TGE::ShapeBase *>(thisptr);
	originalRenderImage(thisptr, state, image);
	gCurrentRenderingShape = NULL;

	if (currentPass != "fwd") {
		thisptr->setCloakLevel(cloakLevel);
	}
}

//There are so many virtual function calls here I can't bring myself to show this to other people

//Figure out which shape instance and mesh object instance is rendering so we can
// get specific transform data and fix the render matrices
MBX_OVERRIDE_MEMBERFN(void, TGE::TSShapeInstance::render, (TGE::TSShapeInstance *thisptr, const Point3F *scale), originalSIRender) {
	if (gCurrentRenderingShape == NULL) {
		//Not a marble
		originalSIRender(thisptr, scale);
		return;
	}

	gDtsCurrentRenderingShapeInstance = thisptr;
	originalSIRender(thisptr, scale);
	gDtsCurrentRenderingShapeInstance = NULL;
}

MBX_OVERRIDE_MEMBERFN(void, TGE::TSShapeInstance::MeshObjectInstance::render, (TGE::TSShapeInstance::MeshObjectInstance *thisptr, S32 objectDetail, TGE::TSMaterialList *materials), originalMOIRender) {
	if (gDtsCurrentRenderingShapeInstance == NULL) {
		//Not a marble
		originalMOIRender(thisptr, objectDetail, materials);
		return;
	}

	gDtsCurrentRenderingObjectInstance = thisptr;
	originalMOIRender(thisptr, objectDetail, materials);
	gDtsCurrentRenderingObjectInstance = NULL;
}

MBX_OVERRIDE_MEMBERFN(void, TGE::TSMesh::render, (TGE::TSMesh *thisptr, S32 frame, S32 matFrame, TGE::TSMaterialList *materials), originalRender) {
	//Make sure this is part of a marble
	if (gDtsCurrentRenderingObjectInstance == NULL) {
		originalRender(thisptr, frame, matFrame, materials);
		return;
	}
	bool drawDetailCrap = false;
	if (gMeshesChecked.find(thisptr) == gMeshesChecked.end()) {
		for (int i = 0; i < materials->mTextureNames.size(); i++)
		{
			char* matName = materials->mTextureNames[i];
			if (std::find(glowTextures.begin(), glowTextures.end(), std::string(matName)) != glowTextures.end()) {
				drawDetailCrap = true;
				gMeshesToDraw.insert(thisptr);
				break;
			}
		}
		gMeshesChecked.insert(thisptr);
	}
	if (!drawDetailCrap) {
		if (gMeshesToDraw.find(thisptr) != gMeshesToDraw.end())
			drawDetailCrap = true;
	}
	if (!drawDetailCrap) {
		if (currentPass == "fwd")
			originalRender(thisptr, frame, matFrame, materials);
		return;
	}
	//Get the renderer to use for our marble
	DTSRenderer *renderer = nullptr;
	auto found = gDtsRenderers.find(gCurrentRenderingShape->getId());
	if (found == gDtsRenderers.end()) {
		//Don't have a renderer, create one (adds to gMarbleRenderers) and init its mesh
		renderer = createDTSRenderer(gCurrentRenderingShape, thisptr);
		renderer->loadTriangleList(thisptr, materials);
	} else {
		//Use the one we have assigned
		std::unordered_map<TGE::TSMesh*, DTSRenderer*>& innermap = found->second;
		auto found2 = innermap.find(thisptr);
		if (found2 == innermap.end()) {
			//Don't have a renderer, create one (adds to gMarbleRenderers) and init its mesh
			renderer = createDTSRenderer(gCurrentRenderingShape, thisptr);
			renderer->loadTriangleList(thisptr, materials);
		}
		else {
			renderer = found2->second;
		}
	}
	//If we're not ready yet, don't start yet
	if (!renderer->canRender() || gNoShaders) {
		if (currentPass == "fwd")
			originalRender(thisptr, frame, matFrame, materials);
		return;
	}
	//Render it!
	renderer->renderDTS(gDtsCurrentRenderingShapeInstance, gDtsCurrentRenderingObjectInstance, thisptr, materials, gCurrentRenderingShape);
}

void cleanupDtsRenderers() {
	for (auto &pair : gDtsRenderers) {
		for (auto& pair2 : pair.second) {
			if (pair2.second)
				delete pair2.second;
		}
	}
	gDtsRenderers.clear();
}

MBX_CONSOLE_FUNCTION(reloadDts, void, 1,1, "") {
	cleanupDtsRenderers();
	//Renderer will be recreated the next time any marble is rendered
}

MBX_CONSOLE_METHOD(ShapeBase, reloadShader, void, 2, 2, "") {
	auto found = gDtsRenderers.find(object->getId());
	if (found != gDtsRenderers.end()) {
		for (auto& pair2 : found->second) {
			if (pair2.second)
				delete pair2.second;
		}
		gDtsRenderers.erase(found);
	}
	gMeshesChecked.clear();
	gMeshesToDraw.clear();
}

MBX_ON_GL_CONTEXT_DESTROY(dtsRenderContextDestroyed, ()) {
	cleanupDtsRenderers();
}
