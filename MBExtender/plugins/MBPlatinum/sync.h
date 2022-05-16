#pragma once
#include <TorqueLib/sim/netObject.h>
typedef S32 SyncId;


SyncId getSyncId(TGE::NetObject* object);

SimObjectId getClientSyncObject(SyncId ghostId);