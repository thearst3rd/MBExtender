//-----------------------------------------------------------------------------
// RTAAutosplitter.cpp
// LiveSplit/LiveSplit One Autosplitter for RTA Speedrun Mode
//
// Copyright (c) 2023 The Platinum Team
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

#include <string>

#include <MBExtender/MBExtender.h>
#include <TorqueLib/console/console.h>
#include <MathLib/MathLib.h>

using namespace std;

// This class is intended to be easy to scan using a LiveSplit autosplitter. We initialize a bugger with a header which
// should be easy to scan for, and the memory locations for all other variables we want to keep track of are known.

MBX_MODULE(RTAAutosplitter);

struct RTAAutosplitterData {
	char header[16];
	bool isEnabled;
	bool isDone;
	bool shouldStartRun;
	bool isPauseScreenOpen;
	// 0x14-17 are reserved boolean flags for somethin idk, and yes I'm using full bytes for each of them for now
	bool reservedBools[4];
	S64 time; // in milliseconds
	S64 lastSplitTime;
	S64 missionTypeBeganTime;
	S64 currentGameBeganTime;
	char* currentMission;
} rtaData;

bool initPlugin(MBX::Plugin& plugin)
{
	MBX_INSTALL(plugin, RTAAutosplitter);

	memset(&rtaData, 0, sizeof(rtaData));

	// Load header with a searchable value. I DON'T declare the entire value as a string, but instead dynamcially
	// generate some of it, so that a sigscan won't find the static string part of it. Is this dumb? Probably lol
	memcpy(rtaData.header, "pqrtaas_", 9);
	for (int i = 8; i < 15; i++) {
		rtaData.header[i] = 'a' + (i - 8);
	}

	rtaData.currentMission = strdup("");

	return true;
}

MBX_CONSOLE_FUNCTION(RTAAS_setIsEnabled, void, 2, 2, "RTAAS_setIsEnabled(isEnabled)")
{
	rtaData.isEnabled = StringMath::scan<bool>(argv[1]);
	TGE::Con::printf("[RTAAutosplitter] isEnabled set to %s", rtaData.isEnabled ? "true" : "false");
};

MBX_CONSOLE_FUNCTION(RTAAS_setIsDone, void, 2, 2, "RTAAS_setIsDone(isDone)")
{
	rtaData.isDone = StringMath::scan<bool>(argv[1]);
	TGE::Con::printf("[RTAAutosplitter] isDone set to %s", rtaData.isDone ? "true" : "false");
};

MBX_CONSOLE_FUNCTION(RTAAS_setShouldStartRun, void, 2, 2, "RTAAS_setShouldStartRun(shouldStartRun)")
{
	rtaData.shouldStartRun = StringMath::scan<bool>(argv[1]);
	TGE::Con::printf("[RTAAutosplitter] shouldStartRun set to %s", rtaData.shouldStartRun ? "true" : "false");
};

MBX_CONSOLE_FUNCTION(RTAAS_setIsPauseScreenOpen, void, 2, 2, "RTAAS_setIsPauseScreenOpen(isPauseScreenOpen)")
{
	rtaData.isPauseScreenOpen = StringMath::scan<bool>(argv[1]);
	TGE::Con::printf("[RTAAutosplitter] isPauseScreenOpen set to %s", rtaData.isPauseScreenOpen ? "true" : "false");
};

MBX_CONSOLE_FUNCTION(RTAAS_setTime, void, 2, 2, "RTAAS_setTime(timeInMs)")
{
	rtaData.time = StringMath::scan<S64>(argv[1]);
	//TGE::Con::printf("[RTAAutosplitter] time set to %lli", rtaData.time);
};

MBX_CONSOLE_FUNCTION(RTAAS_setLastSplitTime, void, 2, 2, "RTAAS_setLastSplitTime(timeInMs)")
{
	rtaData.lastSplitTime = StringMath::scan<S64>(argv[1]);
	TGE::Con::printf("[RTAAutosplitter] lastSplitTime set to %lli", rtaData.lastSplitTime);
};

MBX_CONSOLE_FUNCTION(RTAAS_setMissionTypeBeganTime, void, 2, 2, "RTAAS_setMissionTypeBeginTime(timeInMs)")
{
	rtaData.missionTypeBeganTime = StringMath::scan<S64>(argv[1]);
	TGE::Con::printf("[RTAAutosplitter] missionTypeBeganTime set to %lli", rtaData.missionTypeBeganTime);
};

MBX_CONSOLE_FUNCTION(RTAAS_setCurrentGameBeganTime, void, 2, 2, "RTAAS_setCurrentGameBeganTime(timeInMs)")
{
	rtaData.currentGameBeganTime = StringMath::scan<S64>(argv[1]);
	TGE::Con::printf("[RTAAutosplitter] currentGameBeganTime set to %lli", rtaData.currentGameBeganTime);
};

MBX_CONSOLE_FUNCTION(RTAAS_setCurrentMission, void, 2, 2, "RTAAS_setCurrentMission(currentMission)")
{
	if (rtaData.currentMission)
		free(rtaData.currentMission);
	rtaData.currentMission = strdup(argv[1]);
	TGE::Con::printf("[RTAAutosplitter] currentMission set to %s", rtaData.currentMission);
};
