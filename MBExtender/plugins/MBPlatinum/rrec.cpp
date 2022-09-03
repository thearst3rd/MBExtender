#include "rrec.h"
#include <MBExtender/MBExtender.h>
#include <TorqueLib/console/console.h>
#include <TorqueLib/core/resManager.h>
#include <TorqueLib/core/stream.h>
#include <TorqueLib/core/fileStream.h>
#include <ctype.h>

MBX_MODULE(RRECExtension)

static bool pathComponentsAreEqual(const char* lhs, const char* rhs)
{
	auto lhsDone = false;
	auto rhsDone = false;
	while (true)
	{
		lhsDone = (*lhs == '\0' || *lhs == '/' || *lhs == '\\');
		rhsDone = (*rhs == '\0' || *rhs == '/' || *rhs == '\\');
		if (lhsDone || rhsDone)
			return lhsDone == rhsDone;
		if (tolower(*lhs) != tolower(*rhs))
			return false;
		lhs++;
		rhs++;
	}
}

static const char* nextPathComponent(const char* path)
{
	while (*path && *path != '/' && *path != '\\')
		path++;
	if (!*path)
		return nullptr;
	return path + 1; // Skip the slash after the component
}

/**
 * Check if the game should be allowed to access a path.
 * @arg path The path to check
 * @return Whether the path can be accessed
 */
static bool pathCheck(const char* path)
{
	// Disallow absolute paths
	if (path[0] == '/')
		return false;
	if (path[0] && path[1] == ':')
		return false;

	// Process . and .. components in the path
	auto component = path;
	auto level = 0;
	while (component)
	{
		if (pathComponentsAreEqual(component, ".."))
		{
			// Go up one level if we can
			if (level == 0)
				return false;
			level--;
		}
		else if (!pathComponentsAreEqual(component, "") && !pathComponentsAreEqual(component, "."))
		{
			// Empty components and . keep the path at the current level
			level++;
		}
		component = nextPathComponent(component);
	}

	// The path is only valid if it contains at least one meaningful component
	return level > 0;
}

MBX_CONSOLE_FUNCTION(editRREC, void, 6, 6, "editRREC(path, destpath, name, author, desc)")
{
	if (!pathCheck(argv[1]))
		return;

	char path[256];
	TGE::Con::expandScriptFilename(path, 256, argv[1]);

	char destpath[256];
	TGE::Con::expandScriptFilename(destpath, 256, argv[2]);

	TGE::ResourceObject* rrecfileres = TGE::ResourceManager->find(path);
	if (!rrecfileres)
		return;

	TGE::Stream* rstream = TGE::ResourceManager->openStream(rrecfileres);

	RREC rrec;
	rrec.read(rstream);

	TGE::ResourceManager->closeStream(rstream);

	rrec.name = argv[3];
	rrec.author = argv[4];
	rrec.desc = argv[5];

	TGE::FileStream* stream = TGE::FileStream::create();
	if (!TGE::ResourceManager->openFileForWrite(*stream, destpath, 0x1)) {
		TGE::Con::errorf("Could not Edit Replay");
		return;
	}

	rrec.write(stream);
	stream->close();

	delete stream;
}