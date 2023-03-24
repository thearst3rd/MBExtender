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

static char header[20] = { 0 };
static bool isEnabled = false;
static bool isDone = false;
static bool shouldStartRun = false;
static S64 time = 0; // in milliseconds
static char* currentMission = nullptr;

bool initPlugin(MBX::Plugin& plugin)
{
	MBX_INSTALL(plugin, RTAAutosplitter);

	// Load header with a searchable value. I DON'T declare the entire value as a string, but instead dynamcially
	// generate some of it, so that a sigscan won't find the static string part of it. Is this dumb? Probably lol
	memcpy(header, "pq_rta_as_", 10);
	for (int i = 10; i < 19; i++) {
		header[i] = 'a' + (i - 10);
	}

	currentMission = strdup("");

	return true;
}

MBX_CONSOLE_FUNCTION(RTAAS_setIsEnabled, void, 2, 2, "RTAAS_setIsEnabled(isEnabled)")
{
	isEnabled = (strcmp(argv[1], "true") == 0 || atoi(argv[1]) == 1);
	TGE::Con::printf("[RTAAutosplitter] isEnabled set to %s", isEnabled ? "true" : "false");
};

MBX_CONSOLE_FUNCTION(RTAAS_setIsDone, void, 2, 2, "RTAAS_setIsDone(isDone)")
{
	isDone = (strcmp(argv[1], "true") == 0 || atoi(argv[1]) == 1);
	TGE::Con::printf("[RTAAutosplitter] isDone set to %s", isDone ? "true" : "false");
};

MBX_CONSOLE_FUNCTION(RTAAS_setShouldStartRun, void, 2, 2, "RTAAS_setShouldStartRun(shouldStartRun)")
{
	shouldStartRun = (strcmp(argv[1], "true") == 0 || atoi(argv[1]) == 1);
	TGE::Con::printf("[RTAAutosplitter] shouldStartRun set to %s", shouldStartRun ? "true" : "false");
};

MBX_CONSOLE_FUNCTION(RTAAS_setTime, void, 2, 2, "RTAAS_setTime(timeInMs)")
{
	time = atoll(argv[1]);
	TGE::Con::printf("[RTAAutosplitter] time set to %lli", time);
};

MBX_CONSOLE_FUNCTION(RTAAS_setCurrentMission, void, 2, 2, "RTAAS_setCurrentMission(currentMission)")
{
	if (currentMission)
		free(currentMission);
	currentMission = strdup(argv[1]);
	TGE::Con::printf("[RTAAutosplitter] currentMission set to %s", currentMission);
};
