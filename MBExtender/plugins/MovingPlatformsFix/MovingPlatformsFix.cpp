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

#include <string>

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

void updateMarbleMetrics(TGE::Marble* thisObj);

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
	} else if (!thisObj->mControllable) { //Simulate non-controllable marbles, but don't update platforms
		gSimulatePathedInteriors = false;
		//findContacts();
		originalAdvancePhysics(thisObj, move, delta);
		gSimulatePathedInteriors = true;
	}

	gAdvancingMarble = NULL;
	gVelocityCache.clear();
	updateMarbleMetrics(thisObj);
}

void updateMarbleMetrics(TGE::Marble* thisObj) {
	TGE::Con::setVariable("$MarbleContacts", StringMath::print(thisObj->mContacts.size()));
	for (U32 i = 0; i < thisObj->mContacts.size(); i ++) {
//		TGE::Con::setVariable(std::string("$MarbleContactObject" + std::to_string(i)).c_str(), thisObj->mContacts[i].object ? thisObj->mContacts[i].object->getIdString() : "0");
		TGE::Con::setVariable(std::string("$MarbleContactPosition" + std::to_string(i)).c_str(), StringMath::print(thisObj->mContacts[i].position));
		TGE::Con::setVariable(std::string("$MarbleContactNormal" + std::to_string(i)).c_str(), StringMath::print(thisObj->mContacts[i].normal));
		TGE::Con::setVariable(std::string("$MarbleContactActualNormal" + std::to_string(i)).c_str(), StringMath::print(thisObj->mContacts[i].actualNormal));
		TGE::Con::setVariable(std::string("$MarbleContactSurfaceVelocity" + std::to_string(i)).c_str(), StringMath::print(thisObj->mContacts[i].surfaceVelocity));
		TGE::Con::setVariable(std::string("$MarbleContactSurfaceFrictionVelocity" + std::to_string(i)).c_str(), StringMath::print(thisObj->mContacts[i].surfaceFrictionVelocity));
		TGE::Con::setVariable(std::string("$MarbleContactStaticFriction" + std::to_string(i)).c_str(), StringMath::print(thisObj->mContacts[i].staticFriction));
		TGE::Con::setVariable(std::string("$MarbleContactKineticFriction" + std::to_string(i)).c_str(), StringMath::print(thisObj->mContacts[i].kineticFriction));
		TGE::Con::setVariable(std::string("$MarbleContactVAtC" + std::to_string(i)).c_str(), StringMath::print(thisObj->mContacts[i].vAtC));
		TGE::Con::setVariable(std::string("$MarbleContactVAtCMag" + std::to_string(i)).c_str(), StringMath::print(thisObj->mContacts[i].vAtCMag));
		TGE::Con::setVariable(std::string("$MarbleContactNormalForce" + std::to_string(i)).c_str(), StringMath::print(thisObj->mContacts[i].normalForce));
		TGE::Con::setVariable(std::string("$MarbleContactContactDistance" + std::to_string(i)).c_str(), StringMath::print(thisObj->mContacts[i].contactDistance));
		TGE::Con::setVariable(std::string("$MarbleContactFriction" + std::to_string(i)).c_str(), StringMath::print(thisObj->mContacts[i].friction));
		TGE::Con::setVariable(std::string("$MarbleContactRestitution" + std::to_string(i)).c_str(), StringMath::print(thisObj->mContacts[i].restitution));
		TGE::Con::setVariable(std::string("$MarbleContactForce" + std::to_string(i)).c_str(), StringMath::print(thisObj->mContacts[i].force));
		TGE::Con::setVariable(std::string("$MarbleContactMaterial" + std::to_string(i)).c_str(), StringMath::print(thisObj->mContacts[i].material));
	}
//	TGE::Con::setVariable("$MarbleBestContactObject", thisObj->mBestContact.object ? thisObj->mBestContact.object->getIdString() : "0");
	TGE::Con::setVariable("$MarbleBestContactPosition", StringMath::print(thisObj->mBestContact.position));
	TGE::Con::setVariable("$MarbleBestContactNormal", StringMath::print(thisObj->mBestContact.normal));
	TGE::Con::setVariable("$MarbleBestContactActualNormal", StringMath::print(thisObj->mBestContact.actualNormal));
	TGE::Con::setVariable("$MarbleBestContactSurfaceVelocity", StringMath::print(thisObj->mBestContact.surfaceVelocity));
	TGE::Con::setVariable("$MarbleBestContactSurfaceFrictionVelocity", StringMath::print(thisObj->mBestContact.surfaceFrictionVelocity));
	TGE::Con::setVariable("$MarbleBestContactStaticFriction", StringMath::print(thisObj->mBestContact.staticFriction));
	TGE::Con::setVariable("$MarbleBestContactKineticFriction", StringMath::print(thisObj->mBestContact.kineticFriction));
	TGE::Con::setVariable("$MarbleBestContactVAtC", StringMath::print(thisObj->mBestContact.vAtC));
	TGE::Con::setVariable("$MarbleBestContactVAtCMag", StringMath::print(thisObj->mBestContact.vAtCMag));
	TGE::Con::setVariable("$MarbleBestContactNormalForce", StringMath::print(thisObj->mBestContact.normalForce));
	TGE::Con::setVariable("$MarbleBestContactContactDistance", StringMath::print(thisObj->mBestContact.contactDistance));
	TGE::Con::setVariable("$MarbleBestContactFriction", StringMath::print(thisObj->mBestContact.friction));
	TGE::Con::setVariable("$MarbleBestContactRestitution", StringMath::print(thisObj->mBestContact.restitution));
	TGE::Con::setVariable("$MarbleBestContactForce", StringMath::print(thisObj->mBestContact.force));
	TGE::Con::setVariable("$MarbleBestContactMaterial", StringMath::print(thisObj->mBestContact.material));
//	TGE::Con::setVariable("$MarbleLastContactObject", thisObj->mLastContact.object ? thisObj->mLastContact.object->getIdString() : "0");
	TGE::Con::setVariable("$MarbleLastContactPosition", StringMath::print(thisObj->mLastContact.position));
	TGE::Con::setVariable("$MarbleLastContactNormal", StringMath::print(thisObj->mLastContact.normal));
	TGE::Con::setVariable("$MarbleLastContactActualNormal", StringMath::print(thisObj->mLastContact.actualNormal));
	TGE::Con::setVariable("$MarbleLastContactSurfaceVelocity", StringMath::print(thisObj->mLastContact.surfaceVelocity));
	TGE::Con::setVariable("$MarbleLastContactSurfaceFrictionVelocity", StringMath::print(thisObj->mLastContact.surfaceFrictionVelocity));
	TGE::Con::setVariable("$MarbleLastContactStaticFriction", StringMath::print(thisObj->mLastContact.staticFriction));
	TGE::Con::setVariable("$MarbleLastContactKineticFriction", StringMath::print(thisObj->mLastContact.kineticFriction));
	TGE::Con::setVariable("$MarbleLastContactVAtC", StringMath::print(thisObj->mLastContact.vAtC));
	TGE::Con::setVariable("$MarbleLastContactVAtCMag", StringMath::print(thisObj->mLastContact.vAtCMag));
	TGE::Con::setVariable("$MarbleLastContactNormalForce", StringMath::print(thisObj->mLastContact.normalForce));
	TGE::Con::setVariable("$MarbleLastContactContactDistance", StringMath::print(thisObj->mLastContact.contactDistance));
	TGE::Con::setVariable("$MarbleLastContactFriction", StringMath::print(thisObj->mLastContact.friction));
	TGE::Con::setVariable("$MarbleLastContactRestitution", StringMath::print(thisObj->mLastContact.restitution));
	TGE::Con::setVariable("$MarbleLastContactForce", StringMath::print(thisObj->mLastContact.force));
	TGE::Con::setVariable("$MarbleLastContactMaterial", StringMath::print(thisObj->mLastContact.material));
	TGE::Con::setVariable("$MarbleData_928", StringMath::print(thisObj->data_928));
	TGE::Con::setVariable("$MarbleDeltaPos", StringMath::print(thisObj->delta.pos));
	TGE::Con::setVariable("$MarbleDeltaPosVec", StringMath::print(thisObj->delta.posVec));
	TGE::Con::setVariable("$MarbleDeltaPrevMouseX", StringMath::print(thisObj->delta.prevMouseX));
	TGE::Con::setVariable("$MarbleDeltaPrevMouseY", StringMath::print(thisObj->delta.prevMouseY));
//	TGE::Con::setVariable("$MarbleDataBlock", thisObj->mDataBlock ? thisObj->mDataBlock->getIdString() : "0");
	TGE::Con::setVariable("$MarblePositionKey", StringMath::print(thisObj->mPositionKey));
	TGE::Con::setVariable("$MarbleData_9c0", StringMath::print(thisObj->data_9c0));
	TGE::Con::setVariable("$MarbleBounceEmitDelay", StringMath::print(thisObj->mBounceEmitDelay));
	TGE::Con::setVariable("$MarblePowerUpId", StringMath::print(thisObj->mPowerUpId));
	TGE::Con::setVariable("$MarblePowerUpTimer", StringMath::print(thisObj->mPowerUpTimer));
	TGE::Con::setVariable("$MarbleData_9d0", StringMath::print(thisObj->data_9d0));
	TGE::Con::setVariable("$MarbleMode", StringMath::print(thisObj->mMode));
	TGE::Con::setVariable("$MarbleDataRollingHardSound", StringMath::print(thisObj->mRollingHardSound));
	TGE::Con::setVariable("$MarbleDataRollingSoftSound", StringMath::print(thisObj->mRollingSoftSound));
	TGE::Con::setVariable("$MarbleRadius", StringMath::print(thisObj->mRadius));
	TGE::Con::setVariable("$MarbleGravityUp", StringMath::print(thisObj->mGravityUp));
	TGE::Con::setVariable("$MarbleVelocity", StringMath::print(thisObj->mVelocity));
	TGE::Con::setVariable("$MarblePosition", StringMath::print(thisObj->mPosition));
	TGE::Con::setVariable("$MarbleOmega", StringMath::print(thisObj->mOmega));
	TGE::Con::setVariable("$MarbleCameraYaw", StringMath::print(thisObj->mCameraYaw));
	TGE::Con::setVariable("$MarbleCameraPitch", StringMath::print(thisObj->mCameraPitch));
	TGE::Con::setVariable("$MarbleMouseZ", StringMath::print(thisObj->mMouseZ));
	TGE::Con::setVariable("$MarbleGroundTime", StringMath::print(thisObj->mGroundTime));
	TGE::Con::setVariable("$MarbleControllable", StringMath::print(thisObj->mControllable));
	TGE::Con::setVariable("$MarbleOOB", StringMath::print(thisObj->mOOB));
	TGE::Con::setVariable("$MarbleOOBCamPos", StringMath::print(thisObj->mOOBCamPos));
	TGE::Con::setVariable("$MarbleData_a68", StringMath::print(thisObj->data_a68));
	TGE::Con::setVariable("$MarbleServerMarbleId", StringMath::print(thisObj->mServerMarbleId));
	TGE::Con::setVariable("$MarbleData_a70", StringMath::print(thisObj->data_a70));
//	TGE::Con::setVariable("$MarblePadPtr", thisObj->mPadPtr ? thisObj->mPadPtr->getIdString() : "0");
	TGE::Con::setVariable("$MarbleOnPad", StringMath::print(thisObj->mOnPad));
	TGE::Con::setVariable("$MarbleMaterialCollisions", StringMath::print(thisObj->mMaterialCollisions.size()));
	for (U32 i = 0; i < thisObj->mMaterialCollisions.size(); i++) {
		TGE::Con::setVariable(std::string("$MarbleMaterialCollisionGhostIndex" + std::to_string(i)).c_str(), StringMath::print(thisObj->mMaterialCollisions[i].ghostIndex));
		TGE::Con::setVariable(std::string("$MarbleMaterialCollisionMaterialId" + std::to_string(i)).c_str(), StringMath::print(thisObj->mMaterialCollisions[i].materialId));
		TGE::Con::setVariable(std::string("$MarbleMaterialCollisionAlsoGhostIndex" + std::to_string(i)).c_str(), StringMath::print(thisObj->mMaterialCollisions[i].alsoGhostIndex));
	}
	for (U32 i = 0; i < 6; i ++) {
		TGE::Con::setVariable(std::string("$MarblePowerUpStateActive" + std::to_string(i)).c_str(), StringMath::print(thisObj->mPowerUpState[i].active));
		TGE::Con::setVariable(std::string("$MarblePowerUpStateEndTime" + std::to_string(i)).c_str(), StringMath::print(thisObj->mPowerUpState[i].endTime));
//		TGE::Con::setVariable(std::string("$MarblePowerUpStateEmitter" + std::to_string(i)).c_str(), thisObj->mPowerUpState[i].emitter ? thisObj->mPowerUpState[i].emitter->getIdString() : "0");
	}
//	TGE::Con::setVariable("$MarbleTrailEmitter", thisObj->mTrailEmitter ? thisObj->mTrailEmitter->getIdString() : "0");
	TGE::Con::setVariable("$MarblePredictionTicks", StringMath::print(thisObj->mPredictionTicks));
	TGE::Con::setVariable("$MarbleCameraOffset", StringMath::print(thisObj->mCameraOffset));
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

	TGE::MarbleData *data = static_cast<TGE::MarbleData *>(gAdvancingMarble->mDataBlock);
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
	thisObj->mContacts.clear();

	if (mask != 0) {
		Box3F box(thisObj->mPosition.toPoint3F(), thisObj->mPosition.toPoint3F());
		box.minExtents += thisObj->mObjBox.minExtents - Point3F(0.0001f);
		box.maxExtents += thisObj->mObjBox.maxExtents + Point3F(0.0001f);

		SimpleQueryList queryList;
		thisObj->mContainer->findObjects(box, mask, SimpleQueryList::insertionCallback, &queryList);

		SphereF sphere(thisObj->mPosition.toPoint3F(), thisObj->mRadius);

		thisObj->mPolyList.clear();
		for (int i = 0; i < queryList.mList.size(); i++) {
			queryList.mList[i]->buildPolyList(&thisObj->mPolyList, box, sphere);
		}
	}

	for (int i = 0; i < thisObj->mPolyList.mPolyList.size(); i++) {
		TGE::ConcretePolyList::Poly* poly = &thisObj->mPolyList.mPolyList[i];
		PlaneD plane(poly->plane);
		F64 distance = plane.distToPlane(thisObj->mPosition);
		if (mFabsD(distance) <= thisObj->mRadius + 0.0001) {
			Point3D lastVertex(thisObj->mPolyList.mVertexList[thisObj->mPolyList.mIndexList[poly->vertexStart + poly->vertexCount - 1]]);

			Point3D contactVert = plane.project(thisObj->mPosition);
			F64 separation = mSqrtD(thisObj->mRadius * thisObj->mRadius - distance * distance);

			for (int j = 0; j < poly->vertexCount; j++) {
				Point3D vertex = thisObj->mPolyList.mVertexList[thisObj->mPolyList.mIndexList[poly->vertexStart + j]];
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
			Point3D delta = thisObj->mPosition - contactVert;
			F64 contactDistance = delta.len();
			if ((double)(thisObj->mRadius + 0.0001) < contactDistance) {
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

			const char* contactVertStr = MBX_Strdup(StringMath::print(contactVert));
			const char* normalStr = MBX_Strdup(StringMath::print(normal));
			const char* surfaceVelocityStr = MBX_Strdup(StringMath::print(surfaceVelocity));
			const char* contactDistanceStr = MBX_Strdup(StringMath::print(contactDistance));
			const char* frictionStr = MBX_Strdup(StringMath::print(friction));
			const char* forceStr = MBX_Strdup(StringMath::print(force));
			const char* materialIdStr = MBX_Strdup(StringMath::print(materialId));

			TGE::Con::executef(thisObj, 9, "onCollide", 
					contactVertStr,
					normalStr,
					surfaceVelocityStr,
					contactDistanceStr,
					poly->object->getIdString(),
					frictionStr,
					forceStr,
					materialIdStr
				);
			MBX_Free((void*)contactVertStr);
			MBX_Free((void*)normalStr);
			MBX_Free((void*)surfaceVelocityStr);
			MBX_Free((void*)contactDistanceStr);
			MBX_Free((void*)frictionStr);
			MBX_Free((void*)forceStr);
			MBX_Free((void*)materialIdStr);

			thisObj->mContacts.push_back(contact);

			TGE::GameBase* gb = TGE::TypeInfo::manual_dynamic_cast<TGE::GameBase*>(poly->object, &TGE::TypeInfo::SimObject, &TGE::TypeInfo::GameBase, 0);
			U32 objTypeMask = 0;
			if (gb != nullptr) {
				objTypeMask = gb->mTypeMask;
			}

			// 0x800 is the constant used in code
			if ((objTypeMask & TGE::ShapeBaseObjectType) != 0) {
				U32 netIndex = gb->mNetIndex;

				bool found = false;
				for (int j = 0; j < thisObj->mMaterialCollisions.size(); j++) {
					if (thisObj->mMaterialCollisions[j].ghostIndex == netIndex && thisObj->mMaterialCollisions[j].materialId == materialId) {
						found = true;
						break;
					}
				}

				if (!found) {

					TGE::Marble::MaterialCollision coll{};
					coll.ghostIndex = netIndex;
					coll.materialId = materialId;
					coll.alsoGhostIndex = netIndex;
					thisObj->mMaterialCollisions.push_back(coll);
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
