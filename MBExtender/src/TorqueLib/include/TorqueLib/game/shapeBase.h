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

#include <TorqueLib/core/dnet.h>
#include <TorqueLib/game/gameBase.h>
#include <TorqueLib/sceneGraph/sceneState.h>
#include <TorqueLib/ts/tsShapeInstance.h>

namespace TGE
{
	class BitStream;
	class ShapeBaseData;
	class AudioProfile;
	class TSThread;
	class ShapeBaseImageData;
	class ShapeBaseImageDataStateData;
	class TSShapeInstance;
	class Convex;
	class Shadow;
	class DebrisData;
	class ExplosionData;

	class ShapeBase : public GameBase
	{
		BRIDGE_CLASS(ShapeBase);
	public:
		enum DamageState {
			Enabled,
			Disabled,
			Destroyed,
			NumDamageStates,
			NumDamageStateBits = 2,
		};

		struct Sound {
			bool play;
			U32 timeout;
			AudioProfile *profile;
			void *sound;
		};
		struct Thread {
			enum State {
				Play, Stop, Pause, FromMiddle
			};
			TSThread *thread;
			U32 state;
			S32 sequence;
			U32 sound;
			bool atEnd;
			bool forward;
			char unused[10];
		};
		struct MountedImage {
			ShapeBaseImageData *datablock; // 0
			ShapeBaseImageDataStateData *state; // 4
			ShapeBaseImageData *nextImage; // 8
			StringHandle skinNameHandle; // c
			StringHandle nextSkinNameHandle; // 10

			bool loaded; // 14
			bool nextLoaded; // 15
			F32 delayTime; // 18
			U32 fireCount; // 1c

			bool triggerDown; // 20
			bool ammo; // 21
			bool target; // 22
			bool wet; // 23

			TSShapeInstance *shapeInstance; // 24
			TSThread *ambientThread; // 28
			TSThread *visThread; // 2c
			TSThread *animThread; // 30
			TSThread *flashThread; // 34
			TSThread *spinThread; // 38

			U32 lightStart; // 3c
//			LightInfo mLight; // not included in mbg ?
			bool animLoopingSound; // 40
			void *animSound; // 44
			void *unused[9]; // 48-6c extra space idk
		};
		struct MountInfo {
			ShapeBase *list;
			ShapeBase *object;
			ShapeBase *link;
			U32 node;
		};
		struct CollisionTimeout {
			CollisionTimeout *next;
			ShapeBase *object;
			U32 objectNumber;
			U32 expireTime;
			VectorF vector;
		};

		ShapeBaseData *mDataBlock; // 26c
		GameConnection *mControllingClient; // 270
		ShapeBase *mControllingObject; // 274
		bool mTrigger[6]; // 278

		Sound mSoundThread[4]; // 280
		Thread mScriptThread[4]; // 2c0

		F32 mInvincibleCount; //  330
		F32 mInvincibleTime; // 334
		F32 mInvincibleSpeed; // 338
		F32 mInvincibleDelta; // 33c
		F32 mInvincibleEffect; // 340
		F32 mInvincibleFade; // 344
		bool mInvincibleOn; // 348

		MountedImage mMountedImageList[8]; // 34c
		TSShapeInstance* mShapeInstance; // 6ac
		Convex *mConvexList; // 6b0
		Shadow *mShadow; // 6b4
		bool mGenerateShadow; // 6b8
		StringHandle mSkinNameHandle; // 6bc
		StringHandle mShapeNameHandle; // 6c0

		F32 mEnergy; // 6c4
		F32 mRechargeRate; // 6c8
		bool mChargeEnergy; // 6cc

		F32 mMass; // 6d0
		F32 mOneOverMass; // 6d4

		F32 mDrag; // 6d8
		F32 mBuoyancy; // 6dc
		U32 mLiquidType; // 6e0
		F32 mLiquidHeight; // 6e4
		F32 mWaterCoverage; // 6e8

		Point3F mAppliedForce; // 6ec
		F32 mGravityMod; // 6f8

		F32 mDamageFlash; // 6fc
		F32 mWhiteOut; // 700
		void *data_704; // 704

		bool mFlipFadeVal; // 708
		U32 mLightTime; // 70c

		Point3F mShieldNormal; // 710

		MountInfo mMount; // 71c

		CollisionTimeout *mTimeoutList; // 72c
		F32 mDamage; // 730
		F32 mRepairRate; // 734
		F32 mRepairReserve; // 738
		bool mRepairDamage; // 73c
		DamageState mDamageState; // 740
		TSThread *mDamageThread; // 744
		TSThread *mHulkThread; // 748
		VectorF damageDir; // 74c
		bool blowApart; // 758

		bool mCloaked; // 759
		F32 mCloakLevel; // 75c
		TextureHandle mCloakTexture; // 760
		bool mHidden; // 764

		bool mFadeOut; // 765
		bool mFading; // 766
		F32 mFadeVal; // 768
		F32 mFadeElapsedTime; // 76c
		F32 mFadeTime; // 770
		F32 mFadeDelay; // 774

		F32 mCameraFov; // 778
		bool mIsControlled; // 77c

		U32 mLastRenderFrame; // 780
		F32 mLastRenderDistance; // 784
		U32 mSkinHash; // 788

		// Total size: 78c

		MEMBERFN(bool, isHidden, (), 0x405371_win, 0x2BB60_mac);
		UNDEFVIRT(setImage);
		UNDEFVIRT(onImageRecoil);
		UNDEFVIRT(ejectShellCasing);
		UNDEFVIRT(updateDamageLevel);
		UNDEFVIRT(updateDamageState);
		UNDEFVIRT(blowUp);
		UNDEFVIRT(onMount);
		UNDEFVIRT(onUnmount);
		UNDEFVIRT(onImpact_SceneObject_Point3F);
		UNDEFVIRT(onImpact_Point3F);
		UNDEFVIRT(controlPrePacketSend);
		UNDEFVIRT(setEnergyLevel);
		UNDEFVIRT(mountObject);
		UNDEFVIRT(mountImage);
		UNDEFVIRT(unmountImage);
		UNDEFVIRT(getMuzzleVector);
		UNDEFVIRT(getCameraParameters);
		virtual void getCameraTransform(F32 *pos, MatrixF *mat) = 0;
		UNDEFVIRT(getEyeTransform);
		UNDEFVIRT(getRetractionTransform);
		UNDEFVIRT(getMountTransform);
		UNDEFVIRT(getMuzzleTransform);
		UNDEFVIRT(getImageTransform_uint_MatrixF);
		UNDEFVIRT(getImageTransform_uint_int_MatrixF);
		UNDEFVIRT(getImageTransform_uint_constchar_MatrixF);
		UNDEFVIRT(getRenderRetractionTransform);
		UNDEFVIRT(getRenderMountTransform);
		UNDEFVIRT(getRenderMuzzleTransform);
		UNDEFVIRT(getRenderImageTransform_uint_MatrixF);
		UNDEFVIRT(getRenderImageTransform_uint_int_MatrixF);
		UNDEFVIRT(getRenderImageTransform_uint_constchar_MatrixF);
		UNDEFVIRT(getRenderMuzzleVector);
		UNDEFVIRT(getRenderMuzzlePoint);
		UNDEFVIRT(getRenderEyeTransform);
		UNDEFVIRT(getDamageFlash);
		UNDEFVIRT(setDamageFlash);
		UNDEFVIRT(getWhiteOut);
		UNDEFVIRT(setWhiteOut);
		UNDEFVIRT(getInvincibleEffect);
		UNDEFVIRT(setupInvincibleEffect);
		UNDEFVIRT(updateInvincibleEffect);
		UNDEFVIRT(setVelocity);
		UNDEFVIRT(applyImpulse);
		UNDEFVIRT(setControllingClient);
		UNDEFVIRT(setControllingObject);
		UNDEFVIRT(getControlObject);
		UNDEFVIRT(setControlObject);
		UNDEFVIRT(getCameraFov);
		UNDEFVIRT(getDefaultCameraFov);
		UNDEFVIRT(setCameraFov);
		UNDEFVIRT(isValidCameraFov);
		UNDEFVIRT(renderMountedImage);
		UNDEFVIRT(renderImage);
		UNDEFVIRT(calcClassRenderData);
		UNDEFVIRT(onCollision);
		UNDEFVIRT(getSurfaceFriction);
		UNDEFVIRT(getBounceFriction);
		UNDEFVIRT(setHidden);

		MEMBERFN(void, queueCollision, (ShapeBase* object, const VectorF& vec, U32 materialId), 0x40389b_win, 0x97980_mac);

		MEMBERFN(void, renderObject, (SceneState *state, SceneRenderImage *image), 0x4E5CD0_win, 0xA5F00_mac);
		MEMBERFN(void, renderImage, (SceneState *state, SceneRenderImage *image), 0x404084_win, 0xa2c80_mac);
		MEMBERFN(void, renderMountedImage, (SceneState *state, SceneRenderImage *image), 0x404DEF_win, 0xA1B30_mac);

		MEMBERFN(U32, packUpdate, (NetConnection *connection, U32 mask, BitStream *stream), 0x405F8D_win, 0x9E7A0_mac);
		MEMBERFN(void, unpackUpdate, (NetConnection *connection, BitStream *stream), 0x4041A1_win, 0xA46C0_mac);

		MEMBERFN(void, setHidden, (bool hidden), 0x40104B_win, 0x95BD0_mac);

		MEMBERFN(void, updateThread, (Thread& st), 0x406A69_win, 0x9B2C0_mac);

		MEMBERFN(bool, onAdd, (), 0x40234C_win, 0x0A2AF0_mac);
		MEMBERFN(void, onRemove, (), 0x408607_win, 0x9FB70_mac);

		/// Returns the client controling this object
		GameConnection* getControllingClient() { return mControllingClient; }

		/// Returns the object controling this object
		ShapeBase* getControllingObject()   { return mControllingObject; }
	};

	class ShapeBaseData : public GameBaseData
	{
		BRIDGE_CLASS(ShapeBaseData);
	public:
		StringTableEntry shapeName; // 48
		StringTableEntry cloakTexName; // 4c

		DebrisData *debris; // 50
		S32 debrisID; // 54
		StringTableEntry debrisShapeName; // 58
		Resource<TSShape> debrisShape; // 5c

		ExplosionData* explosion; // 60
		S32 explosionID; // 64

		ExplosionData* underwaterExplosion; // 68
		S32 underwaterExplosionID; // 6c

		F32 mass; // 70
		F32 drag; // 74
		F32 density; // 78
		F32 maxEnergy; // 7c
		F32 maxDamage; // 80
		F32 repairRate; // 84

		F32 disabledLevel; // 88
		F32 destroyedLevel; // 8c

		S32 shieldEffectLifetimeMS; // 90

		F32 cameraMaxDist; // 94
		F32 cameraMinDist; // 98

		F32 cameraDefaultFov; // 9c
		F32 cameraMinFov; // a0
		F32 cameraMaxFov; // a4

		Resource<TSShape> shape; // a8
		U32 mCRC; // ac
		bool computeCRC; // b0

		S32 eyeNode; // b4
		S32 cameraNode; // b8
		S32 shadowNode; // bc
		S32 mountPointNode[32]; // c0
		S32 debrisDetail; // 140
		S32 damageSequence; // 144
		S32 hulkSequence; // 148

		U32 data_14c; // 14c
		U32 data_150; // 150
		U32 data_154; // 154

		bool canControl; // 158
		bool canObserve; // 159
		bool observeThroughObject; // 15a

		StringTableEntry hudImageNameFriendly[8]; // 15c
		StringTableEntry hudImageNameEnemy[8]; // 17c
		TextureHandle hudImageFriendly[8]; // 19c
		TextureHandle hudImageEnemy[8]; // 1bc

		bool hudRenderCenter[8]; // 1dc
		bool hudRenderModulated[8]; // 1e4
		bool hudRenderAlways[8]; // 1ec
		bool hudRenderDistance[8]; // 1f4
		bool hudRenderName[8]; // 1fc

		S32 collisionDetails[8]; // 204
		Box3F collisionBounds[8]; // 224
		S32 LOSDetails[8]; // 2e4

		F32 genericShadowLevel; // 304
		F32 noShadowLevel; // 308

		char data_30c; // 30c
		bool emap; // 30d
		bool firstPersonOnly; // 30e
		bool useEyePoint; // 30f
		bool aiAvoidThis; // 310

		bool isInvincible; // 311
		bool renderWhenDestroyed; // 312

		bool inheritEnergyFromMount; // 313

		// Full size: 314

		GETTERFN(const char*, getShapeFile, 0x48);
		MEMBERFN(void, packData, (BitStream *stream), 0xA3620_mac, 0x407F81_win);
		MEMBERFN(void, unpackData, (BitStream *stream), 0xA3C70_mac, 0x405C7C_win);
	};
}
