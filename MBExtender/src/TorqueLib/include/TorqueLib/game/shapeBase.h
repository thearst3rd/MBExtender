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

#include <TorqueLib/game/gameBase.h>
#include <TorqueLib/sceneGraph/sceneState.h>
#include <TorqueLib/ts/tsShapeInstance.h>

namespace TGE
{
	class BitStream;

	struct Thread {
		enum State {
			Play, Stop, Pause, FromMiddle
		};
		TSShapeInstance::TSThread* thread;
		U32 state;


		S32 sequence;
		U32 sound;
		bool atEnd;
		bool forward;
	};

	class ShapeBase : public GameBase
	{
		BRIDGE_CLASS(ShapeBase);
	public:
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

		GETTERFN(F32, getFadeVal, 0x768);
		SETTERFN(F32, setFadeVal, 0x768);

		GETTERFN(F32, getCloakLevel, 0x75C);
		SETTERFN(F32, setCloakLevel, 0x75C);

		MEMBERFN(void, queueCollision, (ShapeBase* object, const VectorF& vec, U32 materialId), 0x40389b_win, 0x97980_mac);

		MEMBERFN(void, renderObject, (SceneState *state, SceneRenderImage *image), 0x4E5CD0_win, 0xA5F00_mac);
		MEMBERFN(void, renderImage, (SceneState *state, SceneRenderImage *image), 0x404084_win, 0xa2c80_mac);
		MEMBERFN(void, renderMountedImage, (SceneState *state, SceneRenderImage *image), 0x404DEF_win, 0xA1B30_mac);

		MEMBERFN(U32, packUpdate, (NetConnection *connection, U32 mask, BitStream *stream), 0x405F8D_win, 0x9E7A0_mac);
		MEMBERFN(void, unpackUpdate, (NetConnection *connection, BitStream *stream), 0x4041A1_win, 0xA46C0_mac);

		MEMBERFN(void, setHidden, (bool hidden), 0x40104B_win, 0x95BD0_mac);

		GETTERFN(bool, getHiddenGetter, 0x764_win, 0x764_mac);
		SETTERFN(bool, setHiddenSetter, 0x764_win, 0x764_mac);

		GETTERFN(Thread::State, getThread1State, 0x2C4_win, 0x2C4_mac);
		SETTERFN(Thread::State, setThread1State, 0x2C4_win, 0x2C4_mac);

		GETTERFN(bool, getThread1Forward, 0x2D1_win, 0x2D1_mac);
		SETTERFN(bool, setThread1Forward, 0x2D1_win, 0x2D1_mac);

		GETTERFN(bool, getThread1AtEnd, 0x2D0_win, 0x2D0_mac);
		SETTERFN(bool, setThread1AtEnd, 0x2D0_win, 0x2D0_mac);


		GETTERFN(TSShapeInstance::TSThread*, getThread1, 0x2C0_win, 0x2C0_mac);
		SETTERFN(TSShapeInstance::TSThread*, setThread1, 0x2C0_win, 0x2C0_mac);

		GETTERFN(TSShapeInstance*, getTSShapeInstance, 0x6AC_win, 0x6AC_mac);

		MEMBERFN(void, updateThread, (Thread& st), 0x406A69_win, 0x9B2C0_mac);

		MEMBERFN(bool, onAdd, (), 0x40234C_win, 0x0A2AF0_mac);
		MEMBERFN(void, onRemove, (), 0x408607_win, 0x9FB70_mac);
	};

	class ShapeBaseData : public GameBaseData
	{
		BRIDGE_CLASS(ShapeBaseData);
	public:
		GETTERFN(const char*, getShapeFile, 0x48);
		MEMBERFN(void, packData, (BitStream *stream), 0xA3620_mac, 0x407F81_win);
		MEMBERFN(void, unpackData, (BitStream *stream), 0xA3C70_mac, 0x405C7C_win);
	};
}
