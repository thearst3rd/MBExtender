#include <MBExtender/MBExtender.h>
#include <TorqueLib/platform/platform.h>
#include <TorqueLib/game/game.h>
#include <stdlib.h>
#include <TorqueLib/platform/event.h>
#include <TorqueLib/game/demoGame.h>

MBX_MODULE(Timeskip);

static bool isTimeskipping = false;
static int timeskipLeft = 0;
static int timeskipDelta = 1000 / 60; // 60 fps

bool initPlugin(MBX::Plugin& plugin)
{
	MBX_INSTALL(plugin, Timeskip);
	return true;
}

MBX_CONSOLE_FUNCTION(timeskip, void, 2, 2, "timeskip(time)")
{
	timeskipLeft = atoi(argv[1]);
	if (timeskipLeft < 0)
	{
		timeskipLeft = 0;
	}
	else
	{
		isTimeskipping = true;
		TGE::gDGLRender = false;
	}
}

MBX_CONSOLE_FUNCTION(setTimeskipFPS, void, 2, 2, "setTimeskipFPS(fps)")
{
	timeskipDelta = fmax(static_cast<double>(1000) / atoi(argv[1]), 1);
}

MBX_OVERRIDE_FN(void, TGE::GameRenderWorld, (), origGameRenderWorld)
{
	if (!isTimeskipping)
		origGameRenderWorld();
}

MBX_OVERRIDE_FN(void, TGE::TimeManager::process, (), originalProcess)
{
	if (!isTimeskipping)
	{
		originalProcess();
		return;
	}

	int deltaT = timeskipDelta;
	if (timeskipLeft < deltaT)
		deltaT = timeskipLeft;

	TGE::TimeEvent ev;
	ev.elapsedTime = deltaT;
	TGE::Game->postEvent(ev);
	timeskipLeft -= deltaT;
	if (timeskipLeft <= 0) {
		timeskipLeft = 0;
		isTimeskipping = false;
		TGE::gDGLRender = true;
	}
}