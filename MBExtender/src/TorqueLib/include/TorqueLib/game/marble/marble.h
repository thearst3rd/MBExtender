//-----------------------------------------------------------------------------
// Copyright (c) 2021 The Platinum Team
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

#pragma once

#include <MBExtender/InteropMacros.h>
#include <TorqueLib/platform/platform.h>

#include <TorqueLib/game/shapeBase.h>
#include <TorqueLib/math/mAngAxis.h>
#include <TorqueLib/math/mBox.h>
#include <TorqueLib/math/mMatrix.h>
#include <TorqueLib/math/mPoint3.h>
#include <TorqueLib/collision/abstractPolyList.h>

namespace TGE
{
	class BitStream;
	class ParticleEmitter;
	class ParticleEmitterData;
	class MarbleData;

	class NetConnection;

	class Marble : public ShapeBase
	{
		BRIDGE_CLASS(Marble);
	public:

		struct SinglePrecision
		{
			Point3F mPosition;
			Point3F mVelocity;
			Point3F mOmega;
		};

		struct Contact
		{
			SimObject *object;
#ifdef _WIN32
			U32 data_4;
#endif
			Point3D position;
			Point3D normal;
			Point3F actualNormal;
#ifdef _WIN32
			U32 data_44;
#endif
			Point3D surfaceVelocity;
			Point3D surfaceFrictionVelocity;
			F64 staticFriction;
			F64 kineticFriction;
			Point3D vAtC;
			F64 vAtCMag;
			F64 normalForce;
			F64 contactDistance;
			F32 friction;
			F32 restitution;
			F32 force;
			U32 material;
		};

		struct StateDelta
		{
			Point3D pos;
			Point3D posVec;
			F32 prevMouseX;
			F32 prevMouseY;
			Move move;
		};

		struct EndPadEffect
		{
			F32 effectTime;
			Point3F lastCamFocus;
		};

		struct ActiveParams
		{
			float airAccel;
			float gravityMod;
			float bounce;
			float repulseMax;
			float repulseDist;
			float massScale;
			float sizeScale;
		};

		struct PowerUpState
		{
			bool active;
			char pad[3];
			F32 endTime;
			ParticleEmitter *emitter;
		};

		// StaticShape material collisions
		struct MaterialCollision
		{
			U32 ghostIndex;
			U32 materialId;
			U32 alsoGhostIndex; // MBU says this is NetObject*

			bool operator==(const MaterialCollision &rhs) const {
				return ghostIndex == rhs.ghostIndex &&
				       materialId == rhs.materialId &&
				       alsoGhostIndex == rhs.alsoGhostIndex;
			}

			bool operator!=(const MaterialCollision &rhs) const {
				return !(rhs == *this);
			}
		};

		Vector<Contact> mContacts; // 78c
		Contact mBestContact; // 798
		Contact mLastContact; // 860 / 858
//		Point3F mMovePath[2];
//		F32 mMovePathTime[2];
//		U32 mMovePathSize;
		Point3D data_928; // 928 / 918
		StateDelta delta; // 940 / 930
//		Point3F mLastRenderPos;
//		Point3F mLastRenderVel;
		MarbleData *mDataBlock; // 9b8 / 9a8
		U32 mPositionKey; // 9bc / 9ac
		bool data_9c0; // 9c0 / 9b0
		U32 mBounceEmitDelay; // 9c4 / 9b4
		U32 mPowerUpId; // 9c8 / 9b8
		U32 mPowerUpTimer; // 9cc / 9b8
		U32 data_9d0; // 9d0 / 9c0
		U32 mMode; // 9d4 / 9c4
//		U32 mModeTimer;
		U32 mRollingHardSound; // 9d8 / 9c8
		U32 mRollingSoftSound; // 9dc / 9cc
		F32 mRadius; // 9e0 / 9d0
#ifdef _WIN32
		U32 data_9e4; // 9e4 / NA
#endif
		Point3D mGravityUp; // 9e8 / 9d4
//		QuatF mGravityFrame;
//		QuatF mGravityRenderFrame;

		Point3D mVelocity; // a00 / 9ec
		Point3D mPosition; // a18 / a04
		Point3D mOmega; // a30 / a1c
		F32 mCameraYaw; // a48 / a34
		F32 mCameraPitch; // a4c / a38
		F32 mMouseZ; // a50 / a3c (unused?)
		U32 mGroundTime; // a54 / a40
		bool mControllable; // a58 / a44
		bool mOOB; // a59 / a45
		Point3F mOOBCamPos; // a5c / a48
		U32 data_a68; // a68 / a54
		SimObjectId mServerMarbleId; // a6c / a58
		U32 data_a70; // a70 / a5c
		SceneObject *mPadPtr; // a74 / a60
		bool mOnPad; // a78 / a64
		Vector<MaterialCollision> mMaterialCollisions; // a7c / a68

//		U32 mSuperSpeedDoneTime;
//		F32 mLastYaw;
//		Point3F mNetSmoothPos;
//		bool mCenteringCamera;
//		F32 mRadsLeftToCenter;
//		F32 mRadsStartingToCenter;
//		U32 mCheckPointNumber;
//		EndPadEffect mEffect;
//		Point3F mLastCamPos;
//		F32 mCameraDist;
//		bool mCameraInit;

		PowerUpState mPowerUpState[6]; // a88 / a74
		ParticleEmitter *mTrailEmitter; // ad0 / abc
		ConcretePolyList mPolyList; // ad4 / ac0
		U32 mPredictionTicks; // c74 / c60
		Point3D mCameraOffset; // c78 / c64
		Point3F mShadowPoints[33]; // c90 / c7c
		bool mShadowGenerated; // e1c / e08

		// Total size: e20 / e0c

		MEMBERFN(U32, packUpdate, (NetConnection *connection, U32 mask, BitStream *stream), 0x40566E_win, 0x260570_mac);
		MEMBERFN(void, unpackUpdate, (NetConnection *connection, BitStream *stream), 0x403382_win, 0x25F9F0_mac);

		MEMBERFN(void, setPosition, (const Point3D &position, const AngAxisF &camera, F32 pitch), 0x40773E_win, 0x253E60_mac);
		MEMBERFN(void, setPositionSimple, (const Point3D &position), 0x407D51_win, 0x2541A0_mac);

		MEMBERFN(void, doPowerUp, (S32 powerUpId), 0x405F51_win, 0x2576B0_mac);

		MEMBERFN(void, getCameraTransform, (F32 *pos, MatrixF *mat), 0x4982D0_win, 0x25B0A0_mac);
		MEMBERFN(void, advancePhysics, (Move *move, U32 delta), 0x40252C_win, 0x25B990_mac);
		MEMBERFN(void, advanceCamera, (Move *move, U32 delta), 0x402ff4_win, 0x256d40_mac);

		MEMBERFN(void, renderImage, (SceneState *state, SceneRenderImage *image), 0x408305_win, 0x253600_mac);

		GETTERFN(ConcretePolyList, getContactsPolyList, 0xAD4_win, 0xAC0_mac);
		MEMBERFN(void, findContacts, (U32 mask), 0x409101_win, 0x259d30_mac);
		MEMBERFN(void, computeFirstPlatformIntersect, (F64* moveTime), 0x401677_win, 0x259890);
	};

	class MarbleData : public ShapeBaseData
	{
		BRIDGE_CLASS(MarbleData);
	public:
		AudioProfile *rollHardSound; // 314
		AudioProfile *slipSound; // 318
		AudioProfile *bounce1; // 31c
		AudioProfile *bounce2; // 320
		AudioProfile *bounce3; // 324
		AudioProfile *bounce4; // 328
		AudioProfile *jumpSound; // 32c
		F32 maxRollVelocity; // 330
		F32 minVelocityBounceSoft; // 334
		F32 minVelocityBounceHard; // 338
		F32 angularAcceleration; // 33c
		F32 brakingAcceleration; // 340
		F32 staticFriction; // 344
		F32 kineticFriction; // 348
		F32 bounceKineticFriction; // 34c
		F32 gravity; // 350
		F32 maxDotSlide; // 354
		F32 bounceRestitution; // 358
		F32 airAcceleration; // 35c
		F32 energyRechargeRate; // 360
		F32 jumpImpulse; // 364
		F32 cameraDistance; // 368
		U32 maxJumpTicks; // 36c
		F32 maxForceRadius; // 370
		F32 minBounceVel; // 374
		F32 minBounceSpeed; // 378
		F32 minTrailSpeed; // 37c
		ParticleEmitterData *bounceEmitter; // 380
		ParticleEmitterData *trailEmitter; // 384
		ParticleEmitterData *powerUpEmitter[6]; // 388
		U32 powerUpTime[6]; // 3a0

		MEMBERFN(void, packData, (BitStream *stream), 0x405D08_win, 0x25ED70_mac);
		GETTERFN(F32, getCollisionRadius, 0x94);
		SETTERFN(F32, setCollisionRadius, 0x94);

		GETTERFN(void *, getJumpSound, 0x32C);
	};

	class MarbleUpdateEvent
	{
		BRIDGE_CLASS(MarbleUpdateEvent);
	public:
		MEMBERFN(void, unpack, (NetConnection *connection, BitStream *stream), 0x405506_win, 0x293500_mac);
		MEMBERFN(void, pack, (NetConnection *connection, BitStream *stream), 0x4072F7_win, 0x293700_mac);
	};

	FN(void, cMarbleSetPosition, (Marble *obj, int argc, const char **argv), 0x4A13C0_win, 0x253FC0_mac);
	FN(void, cSetGravityDir, (SimObject *obj, int argc, const char **argv), 0x4926D0_win, 0x255930_mac);

	GLOBALVAR(MatrixF, gGlobalGravityMatrix, 0x6a9e80_win, 0x3132a0_mac);
	GLOBALVAR(Point3F, gGlobalGravityDir, 0x6A9E70_win, 0x3132EC_mac);
}
