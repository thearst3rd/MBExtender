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

#include <TorqueLib/sim/sceneObject.h>

namespace TGE
{
	class BitStream;
	class GameBaseData;
	class GameConnection;
	class NetConnection;

	class GameBase : public SceneObject
	{
		BRIDGE_CLASS(GameBase);
	public:
		struct Link {
			GameBase *next;
			GameBase *prev;
		};

		GameBaseData *mDataBlock; // 248
		StringTableEntry *mNameTag; // 24c

		U32 mProcessTag; // 250
		Link mProcessLink; // 254
		SimObjectPtr<GameBase> mAfterObject; // 25c
		U32 data_260; // 260
		bool mProcessTick; // 264
		F32 mTickMS; // 268

		// Full size: 26c

		MEMBERFN(U32, packUpdate, (TGE::NetConnection *conn, U32 mask, TGE::BitStream *stream), 0x405191_win, 0xE83A0_mac);
		MEMBERFN(void, unpackUpdate, (TGE::NetConnection *conn, TGE::BitStream *stream), 0x406FF5_win, 0xE8B20_mac);

		UNDEFVIRT(onNewDataBlock);
		UNDEFVIRT(processTick);
		UNDEFVIRT(interpolateTick);
		UNDEFVIRT(advanceTime);
		UNDEFVIRT(advancePhysics);
		UNDEFVIRT(getVelocity);
		UNDEFVIRT(getForce);
		UNDEFVIRT(writePacketData);
		UNDEFVIRT(readPacketData);
		UNDEFVIRT(getPacketDataChecksum);
	};

	class GameBaseData : public SimDataBlock
	{
		BRIDGE_CLASS(GameBaseData);
	};

#ifdef __APPLE__
	GLOBALVAR(bool, gShowBoundingBox, 0x306A18_mac);
#endif // __APPLE__
	GLOBALVAR(bool, gGamePaused, 0x6AC239_win, 0x2DB235_mac);
}
