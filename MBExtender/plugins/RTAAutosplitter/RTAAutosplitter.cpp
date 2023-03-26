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

using namespace std;

// This class is intended to be easy to scan using a LiveSplit autosplitter. We initialize a bugger with a header which
// should be easy to scan for, and the memory locations for all other variables we want to keep track of are known.

MBX_MODULE(RTAAutosplitter);

static U8 buffer[80] = { 0 };

// I explictly layout the entire contents of the buffer so I know exactly where to look in the autosplitter
static char* header = (char*)buffer;
static U8* isEnabled = buffer + 0x10;
static U8* isDone = buffer + 0x11;
static U8* shouldStartRun = buffer + 0x12;
static U8* isPauseScreenOpen = buffer + 0x13;
// 0x14-17 are reserved boolean flags for somethin idk and yes I'm using full bytes for each of them for now I don't care
static S64* time = (S64*)(buffer + 0x18); // in milliseconds
static S64* lastSplitTime = (S64*)(buffer + 0x20);
static S64* missionTypeBeganTime = (S64*)(buffer + 0x28);
static S64* currentGameBeganTime = (S64*)(buffer + 0x30);
static char** currentMission = (char**)(buffer + 0x38);

bool initPlugin(MBX::Plugin& plugin)
{
	MBX_INSTALL(plugin, RTAAutosplitter);

	// Load header with a searchable value. I DON'T declare the entire value as a string, but instead dynamcially
	// generate some of it, so that a sigscan won't find the static string part of it. Is this dumb? Probably lol
	memcpy(header, "pqrtaas_", 9);
	for (int i = 8; i < 15; i++) {
		header[i] = 'a' + (i - 8);
	}

	*currentMission = strdup("");

	return true;
}

MBX_CONSOLE_FUNCTION(RTAAS_setIsEnabled, void, 2, 2, "RTAAS_setIsEnabled(isEnabled)")
{
	*isEnabled = (U8)(strcmp(argv[1], "true") == 0 || atoi(argv[1]) == 1);
	TGE::Con::printf("[RTAAutosplitter] isEnabled set to %s", *isEnabled ? "true" : "false");
};

MBX_CONSOLE_FUNCTION(RTAAS_setIsDone, void, 2, 2, "RTAAS_setIsDone(isDone)")
{
	*isDone = (U8)(strcmp(argv[1], "true") == 0 || atoi(argv[1]) == 1);
	TGE::Con::printf("[RTAAutosplitter] isDone set to %s", *isDone ? "true" : "false");
};

MBX_CONSOLE_FUNCTION(RTAAS_setShouldStartRun, void, 2, 2, "RTAAS_setShouldStartRun(shouldStartRun)")
{
	*shouldStartRun = (U8)(strcmp(argv[1], "true") == 0 || atoi(argv[1]) == 1);
	TGE::Con::printf("[RTAAutosplitter] shouldStartRun set to %s", *shouldStartRun ? "true" : "false");
};

MBX_CONSOLE_FUNCTION(RTAAS_setIsPauseScreenOpen, void, 2, 2, "RTAAS_setIsPauseScreenOpen(isPauseScreenOpen)")
{
	*isPauseScreenOpen = (U8)(strcmp(argv[1], "true") == 0 || atoi(argv[1]) == 1);
	TGE::Con::printf("[RTAAutosplitter] isPauseScreenOpen set to %s", *isPauseScreenOpen ? "true" : "false");
};

MBX_CONSOLE_FUNCTION(RTAAS_setTime, void, 2, 2, "RTAAS_setTime(timeInMs)")
{
	*time = atoll(argv[1]);
	//TGE::Con::printf("[RTAAutosplitter] time set to %lli", *time);
};

MBX_CONSOLE_FUNCTION(RTAAS_setLastSplitTime, void, 2, 2, "RTAAS_setLastSplitTime(timeInMs)")
{
	*lastSplitTime = atoll(argv[1]);
	TGE::Con::printf("[RTAAutosplitter] lastSplitTime set to %lli", *lastSplitTime);
};

MBX_CONSOLE_FUNCTION(RTAAS_setMissionTypeBeganTime, void, 2, 2, "RTAAS_setMissionTypeBeginTime(timeInMs)")
{
	*missionTypeBeganTime = atoll(argv[1]);
	TGE::Con::printf("[RTAAutosplitter] missionTypeBeganTime set to %lli", *missionTypeBeganTime);
};

MBX_CONSOLE_FUNCTION(RTAAS_setCurrentGameBeganTime, void, 2, 2, "RTAAS_setCurrentGameBeganTime(timeInMs)")
{
	*currentGameBeganTime = atoll(argv[1]);
	TGE::Con::printf("[RTAAutosplitter] currentGameBeganTime set to %lli", *currentGameBeganTime);
};

MBX_CONSOLE_FUNCTION(RTAAS_setCurrentMission, void, 2, 2, "RTAAS_setCurrentMission(currentMission)")
{
	if (*currentMission)
		free(*currentMission);
	*currentMission = strdup(argv[1]);
	TGE::Con::printf("[RTAAutosplitter] currentMission set to %s", *currentMission);
};
