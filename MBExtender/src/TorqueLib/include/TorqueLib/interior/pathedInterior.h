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

namespace TGE
{
	struct Move;
	class InteriorResource;
	class PathedInteriorData;

	class PathedInterior : public GameBase
	{
		BRIDGE_CLASS(PathedInterior);
	public:

		struct ShadowVolume {
			U32 start;
			U32 count;
		};

		PathedInteriorData *mDataBlock; // 26c

		StringTableEntry mName; // 270
		S32 mPathIndex; // 274
		Vector<StringTableEntry> mTriggers; // 278
		Point3F mOffset; // 284
		Box3F mExtrudedBox; // 290
		bool mStopped; // 2a8
		Vector<void*> data_2ac; // 2ac
		Vector<void*> data_2b8; // 2b8
		Vector<U32> mShadowVolumeIndices; // 2c4
		Vector<ShadowVolume> mShadowVolumes; // 2d0
		Vector<Point3F> mShadowVolumePoints; // 2dc
		bool mHasComputedNormals; // 2e8
		Point3F mShadowVolume; // 2ec
		U32 data_2f8; // 2f8
		StringTableEntry mInteriorResName; // 2fc
		S32 mInteriorResIndex; // 300
		Resource<InteriorResource> mInteriorRes; // 304
		Interior* mInterior; // 308
		Vector<ColorI> mVertexColorsNormal; // 30c
		Vector<ColorI> mVertexColorsAlarm; // 318
		U32 mLMHandle; // 324
		MatrixF mBaseTransform; // 328
		Point3F mBaseScale; // 368
		U32 mPathKey; // 374
		F64 mCurrentPosition; // 378
		S32 mTargetPosition; // 380
		Point3F mCurrentVelocity; // 384
		PathedInterior *mNextClientPI; // 390

		MEMBERFN(void, advance, (double delta), 0x4075FE_win, 0x24B8B0_mac);
		MEMBERFN(void, computeNextPathStep, (U32 delta), 0x40879C_win, 0x24C6C0_mac);

		MEMBERFN(Point3F, getVelocity, (), 0x566580_win, 0x24B690_mac);

		GETTERFN(MatrixF, getBaseTransform, 0x328);
		SETTERFN(MatrixF, setBaseTransform, 0x328);

		GETTERFN(Point3F, getBaseScale, 0x368);
		SETTERFN(Point3F, setBaseScale, 0x368);

		GETTERFN(Point3F, getVelocity2, 0x384);
		SETTERFN(Point3F, setVelocity, 0x384);

		GETTERFN(F64, getPathPosition, 0x378);
		SETTERFN(F64, setPathPosition, 0x378);

		GETTERFN(S32, getTargetPosition, 0x380);
		SETTERFN(S32, setTargetPosition, 0x380);

		GETTERFN(Point3F, getOffset, 0x284);
		SETTERFN(Point3F, setOffset, 0x284);

		GETTERFN(U32, getPathKey, 0x374);
		SETTERFN(U32, setPathKey, 0x374);
		MEMBERFN(U32, getPathKey2, (), 0x4045A7_win, 0x24B6C0_mac);

		GETTERFN(const char *, getInteriorResource, 0x2FC);
		GETTERFN(U32, getInteriorIndex, 0x300);

		GETTERFN(Interior*, getInterior, 0x308);

		MEMBERFN(void, processTick, (const Move *move), 0x4087F6_win, 0x24B560_mac);
		MEMBERFN(void, renderObject, (SceneState *state, SceneRenderImage *renderImage), 0x5654C0_win, 0x24D320_mac);

		GETTERFN(PathedInterior *, getNext, 0x390);

		MEMBERFN(U32, packUpdate, (TGE::NetConnection* con, U32 mask, TGE::BitStream* stream), 0x40229D_win, 0x024D4A0_mac);
		MEMBERFN(void, unpackUpdate, (TGE::NetConnection* con, TGE::BitStream* stream), 0x4093E0_win, 0x24D740_mac);
		MEMBERFN(bool, onAdd, (), 0x401D34_win, 0x24C3B0_mac);
		MEMBERFN(void, onRemove, (), 0x4010E6_win, 0x24BA30_mac);
	};

	//PathedInterior::mClientPathedInteriors (start of a linked list)
	GLOBALVAR(PathedInterior *, gClientPathedInteriors, 0x6C3BE0_win, 0x2DDBC0_mac);
}
