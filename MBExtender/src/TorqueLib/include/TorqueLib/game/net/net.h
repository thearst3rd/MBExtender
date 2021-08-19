#pragma once

#include <MBExtender/InteropMacros.h>
#include <TorqueLib/platform/platform.h>
#include <TorqueLib/sim/netConnection.h>

namespace TGE
{
	class RemoteCommandEvent : public NetEvent
	{
		BRIDGE_CLASS(RemoteCommandEvent);
	public:
		MEMBERFN(void, process, (NetConnection *conn), 0x401947_win, 0x291070_mac);

		FIELD(int, mArgc, 0x24);
		FIELD(char**, mArgv, 0x28);
	};
}
