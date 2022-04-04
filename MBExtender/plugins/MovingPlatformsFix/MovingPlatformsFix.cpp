//-----------------------------------------------------------------------------
// Copyright (c) 2015 The Platinum Team
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

// Fixes moving platforms in multiplayer

#include <MBExtender/MBExtender.h>
#include <unordered_map>
#include <MathLib/MathLib.h>

#include <TorqueLib/audio/audio.h>
#include <TorqueLib/game/camera.h>
#include <TorqueLib/game/marble/marble.h>
#include <TorqueLib/game/gameConnection.h>
#include <TorqueLib/interior/pathedInterior.h>
#include <TorqueLib/sim/pathManager.h>
#include <MBExtender/InteropMacros.h>
#include <TorqueLib/collision/abstractPolyList.h>
#include <TorqueLib/core/stringTable.h>

#ifdef __APPLE__
#include <mach/vm_map.h>
#include <mach/mach_init.h>
#endif
#include <TorqueLib/TypeInfo.h>

MBX_MODULE(MovingPlatformsFix);

struct MarbleCollision {
	Point3D points[8];
};

struct Contact
{
	Point3F position;
	Point3F normal;
	double distance;
};

class SimpleQueryList
{
public:
	TGE::Vector<TGE::SceneObject*> mList;


	SimpleQueryList() {

	}

	~SimpleQueryList() {

	}

	STATICFN(void, insertionCallback, (TGE::SceneObject*, void* key), 0x0404F1B_win, 0x18E580_mac);
};

static bool gSimulatePathedInteriors = true;
static TGE::Marble *gAdvancingMarble = NULL;
static std::unordered_map<TGE::SceneObject *, Point3F> gVelocityCache;

void advancePathedInteriors(U32 delta) {
	if (!gSimulatePathedInteriors)
		return;

	for (TGE::PathedInterior *walk = TGE::gClientPathedInteriors; walk; walk = walk->getNext()) {
		walk->computeNextPathStep(delta);
	}
	for (TGE::PathedInterior *walk = TGE::gClientPathedInteriors; walk; walk = walk->getNext()) {
		walk->advance(delta);
	}

	gVelocityCache.clear();
}

MBX_CONSOLE_METHOD(Marble, testThing, void, 2, 2, "")
{
	TGE::Marble* marble = static_cast<TGE::Marble*>(TGE::Sim::findObject(argv[1]));
	TGE::Con::printf("Best Contact");
	TGE::Con::printf("Position %s", StringMath::print(marble->mBestContact().position));
	TGE::Con::printf("Normal %s", StringMath::print(marble->mBestContact().normal));
	TGE::Con::printf("ActualNormal %s", StringMath::print(marble->mBestContact().actualNormal));
	TGE::Con::printf("SurfaceVel %s", StringMath::print(marble->mBestContact().surfaceVelocity));
	TGE::Con::printf("SurfaceFrictionVel %s", StringMath::print(marble->mBestContact().surfaceFrictionVelocity));
	TGE::Con::printf("StaticFriction %s", StringMath::print(marble->mBestContact().staticFriction));
	TGE::Con::printf("KineticFriction %s", StringMath::print(marble->mBestContact().kineticFriction));
	TGE::Con::printf("vAtC %s", StringMath::print(marble->mBestContact().vAtC));
	TGE::Con::printf("vAtCMag %s", StringMath::print(marble->mBestContact().vAtCMag));
	TGE::Con::printf("NormalForce %s", StringMath::print(marble->mBestContact().normalForce));
	TGE::Con::printf("ContactDistance %s", StringMath::print(marble->mBestContact().contactDistance));
	TGE::Con::printf("Friction %s", StringMath::print(marble->mBestContact().friction));
	TGE::Con::printf("Restitution %s", StringMath::print(marble->mBestContact().restitution));
	TGE::Con::printf("Force %s", StringMath::print(marble->mBestContact().force));
	TGE::Con::printf("Material %s", StringMath::print(marble->mBestContact().material));

	TGE::Con::printf("Last Contact");
	TGE::Con::printf("Position %s", StringMath::print(marble->mLastContact().position));
	TGE::Con::printf("Normal %s", StringMath::print(marble->mLastContact().normal));
	TGE::Con::printf("ActualNormal %s", StringMath::print(marble->mLastContact().actualNormal));
	TGE::Con::printf("SurfaceVel %s", StringMath::print(marble->mLastContact().surfaceVelocity));
	TGE::Con::printf("SurfaceFrictionVel %s", StringMath::print(marble->mLastContact().surfaceFrictionVelocity));
	TGE::Con::printf("StaticFriction %s", StringMath::print(marble->mLastContact().staticFriction));
	TGE::Con::printf("KineticFriction %s", StringMath::print(marble->mLastContact().kineticFriction));
	TGE::Con::printf("vAtC %s", StringMath::print(marble->mLastContact().vAtC));
	TGE::Con::printf("vAtCMag %s", StringMath::print(marble->mLastContact().vAtCMag));
	TGE::Con::printf("NormalForce %s", StringMath::print(marble->mLastContact().normalForce));
	TGE::Con::printf("ContactDistance %s", StringMath::print(marble->mLastContact().contactDistance));
	TGE::Con::printf("Friction %s", StringMath::print(marble->mLastContact().friction));
	TGE::Con::printf("Restitution %s", StringMath::print(marble->mLastContact().restitution));
	TGE::Con::printf("Force %s", StringMath::print(marble->mLastContact().force));
	TGE::Con::printf("Material %s", StringMath::print(marble->mLastContact().material));
}

MBX_OVERRIDE_MEMBERFN(void, TGE::Marble::computeFirstPlatformIntersect, (TGE::Marble* thisPtr, F64* moveTime), originalComputeFirstPlatformIntersect) {
	originalComputeFirstPlatformIntersect(thisPtr, moveTime);
}

// Hook for Marble::advancePhysics that sets gLocalUpdate to true if a local update is occurring
MBX_OVERRIDE_MEMBERFN(void, TGE::Marble::advancePhysics, (TGE::Marble *thisObj, TGE::Move *move, U32 delta), originalAdvancePhysics)
{
	gAdvancingMarble = thisObj;
	if (TGE::NetConnection::getConnectionToServer() == thisObj->getControllingClient()) {
		//findContacts();
		originalAdvancePhysics(thisObj, move, delta);
	} else if (!thisObj->getControllable()) { //Simulate non-controllable marbles, but don't update platforms
		gSimulatePathedInteriors = false;
		//findContacts();
		originalAdvancePhysics(thisObj, move, delta);
		gSimulatePathedInteriors = true;
	}

	gAdvancingMarble = NULL;
	gVelocityCache.clear();
}

// Hook for Camera::advancePhysics that moves MP if you're not a marble
MBX_OVERRIDE_MEMBERFN(void, TGE::Camera::advancePhysics, (TGE::Camera *thisObj, const TGE::Move *move, U32 delta), originalCameraAdvancePhysics)
{
	if (TGE::NetConnection::getConnectionToServer() == thisObj->getControllingClient()) {
		originalCameraAdvancePhysics(thisObj, move, delta);
		advancePathedInteriors(delta);
	}
	gVelocityCache.clear();
}


// Hook for PathedInterior::computeNextPathStep that only lets the call succeed if a local update is occurring
MBX_OVERRIDE_MEMBERFN(void, TGE::PathedInterior::computeNextPathStep, (TGE::PathedInterior *thisObj, U32 delta), originalComputeNextPathStep)
{
	if (gSimulatePathedInteriors) {
		originalComputeNextPathStep(thisObj, delta);
	}
}

// Hook for PathedInterior::advance that only lets the call succeed if a local update is occurring
MBX_OVERRIDE_MEMBERFN(void, TGE::PathedInterior::advance, (TGE::PathedInterior *thisObj, double delta), originalAdvance)
{
	if (gSimulatePathedInteriors) {
		originalAdvance(thisObj, delta);
	}
}

MBX_CONSOLE_FUNCTION(setSimuatingPathedInteriors, void, 2, 2, "setSimulatingPathedInteriors(bool simulating);") {
	gSimulatePathedInteriors = atoi(argv[1]) != 0;
}

MBX_ON_CLIENT_PROCESS(onProcess, (uint32_t delta)) {
	if (atoi(TGE::Con::getVariable("Server::Dedicated")) || atoi(TGE::Con::getVariable("ManualPathedInteriors"))) {
		advancePathedInteriors(delta);
	}
}

MBX_OVERRIDE_MEMBERFN(TGE::Material*, TGE::SceneObject::getMaterialPropertyUnvirt, (TGE::SceneObject* thisObj, U32 which), originalGetMaterialProperty) {
	TGE::PathedInterior* pi = TGE::TypeInfo::manual_dynamic_cast<TGE::PathedInterior*>(thisObj, &TGE::TypeInfo::SceneObject, &TGE::TypeInfo::PathedInterior, 0);
	
	if (pi && !TGE::Con::getBoolVariable("$NoMovingPlatformFrictions")) {
		const char* name = pi->getInterior()->mMaterialList->mTextureNames[which];
		TGE::MaterialPropertyMap* propMap = (TGE::MaterialPropertyMap*)TGE::Sim::findObject("MaterialPropertyMap");
		return propMap->getMapEntry(name);
		
	}
	return originalGetMaterialProperty(thisObj, which);
}

MBX_CONSOLE_METHOD(PathedInterior, getTargetPosition, S32, 2, 2, "PathedInterior.getTargetPosition() -> gets the interior's target position on its path") {
	return object->getTargetPosition();
}

MBX_CONSOLE_METHOD(PathedInterior, getPathPosition, F32, 2, 2, "PathedInterior.getPathPosition() -> gets the interior's position along its path") {
	return static_cast<F32>(object->getPathPosition());
}

MBX_CONSOLE_METHOD(PathedInterior, setPathPosition2, void, 3, 3, "") {
	F64 position = StringMath::scan<F64>(argv[2]);
	object->setPathPosition(position);
}

MBX_CONSOLE_METHOD(PathedInterior, getPathTotalTime, S32, 2, 2, "") {
	TGE::PathManager *manager = (object->isClientObject() ? TGE::gClientPathManager : TGE::gServerPathManager);
	U32 pathKey = object->getPathKey();
	U32 totalTime = manager->getPathTotalTime(pathKey);
	return totalTime;
}

MBX_OVERRIDE_MEMBERFN(U32, TGE::PathManager::getPathTotalTime, (TGE::PathManager *thisptr, U32 id), originalGetPathTotalTime) {
	if (atoi(TGE::Con::getVariable("Server::Dedicated"))) {
		if (thisptr == TGE::gClientPathManager)
			thisptr = TGE::gServerPathManager;
	}

	return originalGetPathTotalTime(thisptr, id);
}

MBX_OVERRIDE_MEMBERFN(void, TGE::PathedInterior::processTick, (TGE::PathedInterior *thisptr, const TGE::Move *move), originalProcessTick) {
	if (thisptr->isServerObject()) {
		if (!gSimulatePathedInteriors)
			return;

		S32 timeMs = 32;
		S32 mTargetPosition = thisptr->getTargetPosition();
		F64 mCurrentPosition = thisptr->getPathPosition();
		U32 pathKey = thisptr->getPathKey2();

		if(mCurrentPosition != mTargetPosition)
		{
			S32 delta;
			if(mTargetPosition == -1)
				delta = timeMs;
			else if(mTargetPosition == -2)
				delta = -timeMs;
			else
			{
				delta = static_cast<S32>(mTargetPosition - mCurrentPosition);
				if(delta < -timeMs)
					delta = -timeMs;
				else if(delta > timeMs)
					delta = timeMs;
			}
			mCurrentPosition += delta;
			U32 totalTime = TGE::gServerPathManager->getPathTotalTime(pathKey);

			if (mTargetPosition < 0) {
				while(mCurrentPosition > totalTime)
					mCurrentPosition -= totalTime;
				while(mCurrentPosition < 0)
					mCurrentPosition += totalTime;
			} else {
				if (mCurrentPosition > totalTime)
					mCurrentPosition = totalTime;
				if (mCurrentPosition < 0)
					mCurrentPosition = 0;
			}

			thisptr->setPathPosition(mCurrentPosition);
		}
	}
}

MBX_CONSOLE_METHOD(PathedInterior, getTransform, const char *, 2, 2, "getTransform()") {
	MatrixF mat(1);
	if (object->isServerObject()) {
		S32 timeMs = 32;
		F64 mCurrentPosition = object->getPathPosition();
		U32 pathKey = object->getPathKey2();

		Point3F initial, position;
		TGE::gServerPathManager->getPathPosition(pathKey, mCurrentPosition, position);
		TGE::gServerPathManager->getPathPosition(pathKey, 0, initial);

		mat.setPosition(position - initial);
	} else {
		mat = object->getTransform();
	}

	return StringMath::print(mat);
}

MBX_OVERRIDE_FN(int, TGE::alxPlay, (void *profile, MatrixF *mat, Point3F *point), originalAlxPlay) {
	if (gAdvancingMarble == NULL)
		return originalAlxPlay(profile, mat, point);

	TGE::MarbleData *data = static_cast<TGE::MarbleData *>(gAdvancingMarble->getDataBlock());
	if (profile == data->getJumpSound()) {
		TGE::Con::executef(gAdvancingMarble, 1, "onJump");
	}
	return originalAlxPlay(profile, mat, point);
}

std::unordered_map<U32, Point3F> velocityCache;

MBX_CONSOLE_METHOD(SceneObject, getSurfaceVelocity, const char *, 5, 5, "SceneObject.getSurfaceVelocity(Marble contact, Point3F contactPoint, F32 distance) -> override this for custom velocity") {
	return "0 0 0";
}

MBX_CONSOLE_METHOD(PathedInterior, getSurfaceVelocity, const char *, 5, 5, "PathedInterior.getSurfaceVelocity(Marble contact, Point3F contactPoint, F32 distance) -> override this for custom velocity") {
	return StringMath::print(object->getVelocity2());
}

MBX_CONSOLE_METHOD(PathedInterior, getVelocity, const char *, 2, 2, "PathedInterior.getVelocity() -> gets the interior's current velocity") {
	return StringMath::print(object->getVelocity2());
}

MBX_OVERRIDE_MEMBERFN(void, TGE::Marble::findContacts, (TGE::Marble* thisObj, U32 mask), originalFindContacts) {
	thisObj->mContacts().clear();

	if (mask != 0) {
		Box3F box(thisObj->mPosition().toPoint3F(), thisObj->mPosition().toPoint3F());
		box.minExtents += thisObj->mObjBox().minExtents - Point3F(0.0001f);
		box.maxExtents += thisObj->mObjBox().maxExtents + Point3F(0.0001f);

		SimpleQueryList queryList;
		thisObj->mContainer()->findObjects(box, mask, SimpleQueryList::insertionCallback, &queryList);

		SphereF sphere(thisObj->mPosition().toPoint3F(), thisObj->mRadius());

		thisObj->mPolyList().clear();
		for (int i = 0; i < queryList.mList.size(); i++) {
			queryList.mList[i]->buildPolyList(&thisObj->mPolyList(), box, sphere);
		}
	}

	for (int i = 0; i < thisObj->mPolyList().mPolyList.size(); i++) {
		TGE::ConcretePolyList::Poly* poly = &thisObj->mPolyList().mPolyList[i];
		PlaneD plane(poly->plane);
		F64 distance = plane.distToPlane(thisObj->mPosition());
		if (mFabsD(distance) <= thisObj->mRadius() + 0.0001) {
			Point3D lastVertex(thisObj->mPolyList().mVertexList[thisObj->mPolyList().mIndexList[poly->vertexStart + poly->vertexCount - 1]]);

			Point3D contactVert = plane.project(thisObj->mPosition());
			F64 separation = mSqrtD(thisObj->mRadius() * thisObj->mRadius() - distance * distance);

			for (int j = 0; j < poly->vertexCount; j++) {
				Point3D vertex = thisObj->mPolyList().mVertexList[thisObj->mPolyList().mIndexList[poly->vertexStart + j]];
				if (vertex != lastVertex) {
					PlaneD vertPlane(vertex + plane, vertex, lastVertex);
					F64 vertDistance = vertPlane.distToPlane(contactVert);
					if (vertDistance < 0.0) {
						if (vertDistance < -(separation + 0.0001))
							goto superbreak;

						if (PlaneD(vertPlane + vertex, vertex, vertex + plane).distToPlane(contactVert) >= 0.0) {
							if (PlaneD(lastVertex - vertPlane, lastVertex, lastVertex + plane).distToPlane(contactVert) >= 0.0) {
								contactVert = vertPlane.project(contactVert);
								break;
							}
							contactVert = lastVertex;
						}
						else {
							contactVert = vertex;
						}
					}
					lastVertex = vertex;
				}
			}


			TGE::Material* matProp = poly->object->getMaterialProperty(poly->material);

			U32 materialId = poly->material;
			Point3D delta = thisObj->mPosition() - contactVert;
			F64 contactDistance = delta.len();
			if ((double)(thisObj->mRadius() + 0.0001) < contactDistance) {
				continue;
			}

			TGE::PathedInterior* hitPI = TGE::TypeInfo::manual_dynamic_cast<TGE::PathedInterior*>(poly->object, &TGE::TypeInfo::SceneObject, &TGE::TypeInfo::PathedInterior, 0);

			Point3D surfaceVelocity;
			surfaceVelocity = Point3D(0);
			if (hitPI != nullptr) {
				surfaceVelocity = hitPI->getVelocity();
			}
			else {
				const char* point = StringMath::print(contactVert);
				char* p2 = new char[strlen(point) + 1];
				strcpy(p2, point);

				const char* scriptVel = TGE::Con::executef(poly->object, 4, "getSurfaceVelocity", thisObj->getIdString(), p2, StringMath::print(contactDistance));

				delete[] p2;

				//Blank means no response
				if (strlen(scriptVel)) {
					surfaceVelocity = StringMath::scan<Point3F>(scriptVel);
				}
			}

			Point3D normal(plane.x, plane.y, plane.z);
			if (contactDistance != 0.0) {
				normal = delta * (1.0 / contactDistance);
			}
			F32 force = 0.0;
			F32 friction = 1.0;
			F32 restitution = 1.0;
			if (matProp != nullptr) {
				friction = matProp->friction;
				restitution = matProp->restitution;
				force = matProp->force;
			}

			TGE::Marble::Contact contact{};

			contact.restitution = restitution;
			contact.normal = normal;
			contact.position = contactVert;
			contact.surfaceVelocity = surfaceVelocity;
			contact.object = poly->object;
			contact.contactDistance = contactDistance;
			contact.friction = friction;
			contact.force = force;
			contact.material = materialId;

			TGE::Con::executef(thisObj, 9, "onCollide", 
					TGE::StringTable->insert(StringMath::print(contactVert), false),
					TGE::StringTable->insert(StringMath::print(normal), false),
					TGE::StringTable->insert(StringMath::print(surfaceVelocity), false),
					TGE::StringTable->insert(StringMath::print(contactDistance), false),
					TGE::StringTable->insert(poly->object->getIdString(), false),
					TGE::StringTable->insert(StringMath::print(friction), false),
					TGE::StringTable->insert(StringMath::print(force), false),
					TGE::StringTable->insert(StringMath::print(materialId), false)
				);

			thisObj->mContacts().push_back(contact);

			TGE::GameBase* gb = TGE::TypeInfo::manual_dynamic_cast<TGE::GameBase*>(poly->object, &TGE::TypeInfo::SimObject, &TGE::TypeInfo::GameBase, 0);
			U32 objTypeMask = 0;
			if (gb != nullptr) {
				objTypeMask = gb->mTypeMask;
			}

			// 0x800 is the constant used in code
			if ((objTypeMask & TGE::ShapeBaseObjectType) != 0) {
				U32 netIndex = gb->mNetIndex();

				bool found = false;
				for (int j = 0; j < thisObj->mMaterialCollisions().size(); j++) {
					if (thisObj->mMaterialCollisions()[j].ghostIndex == netIndex && thisObj->mMaterialCollisions()[j].materialId == materialId) {
						found = true;
						break;
					}
				}

				if (!found) {

					TGE::Marble::MaterialCollision coll{};
					coll.ghostIndex = netIndex;
					coll.materialId = materialId;
					coll.alsoGhostIndex = netIndex;
					thisObj->mMaterialCollisions().push_back(coll);
					Point3F offset(0, 0, 0);
					thisObj->queueCollision(reinterpret_cast<TGE::ShapeBase*>(gb), offset, materialId);
				}
			}
		}
	superbreak:
		int testshit = 0;
	}
}

bool initPlugin(MBX::Plugin &plugin)
{
	MBX_INSTALL(plugin, MovingPlatformsFix);
	TGE::Con::setBoolVariable("$NoMovingPlatformFrictions", 0);
	return true;
}
