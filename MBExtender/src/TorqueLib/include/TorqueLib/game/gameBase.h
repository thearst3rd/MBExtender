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

	struct Move
	{
		// packed storage rep, set in clamp
		S32 px, py, pz;
		U32 pyaw, ppitch, proll;
		F32 x, y, z;          // float -1 to 1
		F32 yaw, pitch, roll; // 0-2PI
		U32 id;               // sync'd between server & client - debugging tool.
		U32 sendCount;

		bool freeLook;
		bool trigger[4];

	};

	GLOBALVAR(Move, gFirstMove, 0x6ac0b0_win, 0x305e80_mac);
	GLOBALVAR(Move, gNextMove, 0x6ac190_win, 0x305e40_mac);

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
		virtual void advanceTime(U32 delta);
		virtual void advancePhysicsVirt(TGE::Move *move, U32 delta);
		UNDEFVIRT(getVelocity);
		UNDEFVIRT(getForce);
		UNDEFVIRT(writePacketData);
		UNDEFVIRT(readPacketData);
		UNDEFVIRT(getPacketDataChecksum);
	};

	class GameBaseData : public SimDataBlock
	{
		BRIDGE_CLASS(GameBaseData);
	public:
		bool packed; // 34
		StringTableEntry category; // 38
		StringTableEntry className; // 3c
		U32 data_40; // 40
		U32 data_44; // 44

		// Full size: 48
	};

	class ProcessList
	{
		BRIDGE_CLASS(ProcessList);
	public:
		GETTERFN(GameBase, head, 0);

		MEMBERFN(bool, advanceClientTime, (U32 timeDelta), 0x402cac_win, 0_mac);
	};

	GLOBALVAR(TGE::ProcessList, gClientProcessList, 0x6ab428_win, 0x306400_mac);

#ifdef __APPLE__
	GLOBALVAR(bool, gShowBoundingBox, 0x306A18_mac);
#endif // __APPLE__
	GLOBALVAR(bool, gGamePaused, 0x6AC239_win, 0x2DB235_mac);
}
