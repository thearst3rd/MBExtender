#include <MBExtender/MBExtender.h>
#include <MathLib/MathLib.h>
#include <TorqueLib/sim/sceneObject.h>
#include <TorqueLib/core/stringTable.h>
#include <TorqueLib/console/consoleInternal.h>
#include <sstream>
#include <TorqueLib/game/gameBase.h>
#include "sync.h"
#include <TorqueLib/game/item.h>

MBX_MODULE(Interpolation);

static U32 getUnitCount(const char *string, const char *set)
{
	U32 count = 0;
	U8 last = 0;
	while (*string)
	{
		last = *string++;

		for (U32 i = 0; set[i]; i++)
		{
			if (last == set[i])
			{
				count++;
				last = 0;
				break;
			}
		}
	}
	if (last)
		count++;
	return count;
}

static bool isObject(const char *str)
{
	if (!strcmp(str, "0") || !strcmp(str, ""))
		return false;
	else
		return (TGE::Sim::findObject(str) != NULL);
}

// Copied from MBRewind lol
TGE::NamespaceEntry *lookup(TGE::Namespace *nmspc, const char *ns, const char *fn)
{
	TGE::Namespace *nm = nmspc;
	while (nm != NULL)
	{
		bool found = false;
		if (nm->mPackage != NULL) {
			for (int i = 0; i < TGE::mNumActivePackages; i++) {
				if (nm->mPackage == TGE::mActivePackages[i]) {
					found = true;
					break;
				}
			}
		}
		else
			found = true;
		if (found) {
			if (nm->mEntryList != NULL)
			{
				if (nm->mName != NULL)
				{
					if (strcmp(nm->mName, ns) == 0)
					{
						TGE::NamespaceEntry* entry = nm->mEntryList;
						while (entry != NULL)
						{
							if (strcmp(entry->mFunctionName, fn) == 0)
								return entry;
							entry = entry->mNext;
						}
					}
				}
			}
		}
		nm = nm->mNext;
	}
}

const char *executeNamespacedFn(const char *ns, const char *fn, int argc, const char *argv[])
{
	TGE::Namespace *nmspc = TGE::Namespace::find(ns, 0);
	TGE::NamespaceEntry *en = lookup(nmspc, ns, fn); // nmspc->lookup(TGE::StringTable->insert(fn, false));
	return en->execute(argc, argv, &TGE::gEvalState);
}

const char *executefnmspc(const char *ns, const char *fn, S32 argc, ...)
{
	const char *argv[128];

	va_list args;
	va_start(args, argc);
	argv[0] = fn;
	for (S32 i = 0; i < argc; i++)
		argv[i + 1] = va_arg(args, const char *);
	va_end(args);

	return executeNamespacedFn(ns, fn, argc + 1, argv);
}

TGE::SimObject* Node_getNextNode(const char* objId, const char* nodeId);
bool Node_isBranching(const char* objId);
float Node_getPathTime(const char* objId, const char* nodeId);

TGE::SimObject* Node_getNextNode(const char* objId, const char* nodeId) {
	TGE::SceneObject* node = static_cast<TGE::SceneObject*>(TGE::Sim::findObject(nodeId));
	const char* nextNode = node->getDataField("nextNode"_ts, NULL);
	if (isObject(nextNode)) {
		return TGE::Sim::findObject(nextNode);
	}

	const char* _nextNodeId = node->getDataField("_nextNodeId"_ts, NULL);
	if (strcmp(_nextNodeId, "") != 0) {
		const char* sync = TGE::Con::executef(2, "getClientSyncObject", _nextNodeId);
		if (isObject(sync)) {
			return TGE::Sim::findObject(sync);
		}
	}

	if (Node_isBranching(nodeId)) {
		if (!isObject(objId) && !isObject(TGE::Con::executef(2, "getClientSyncObject", objId))) {
			const char* branchNodes = node->getDataField("branchNodes"_ts, NULL);
			std::stringstream ss(branchNodes);
			char line[64];
			ss.getline(line, 64, ' ');
			char* firstWord = line;
			if (isObject(firstWord))
				return TGE::Sim::findObject(firstWord);
			return node;
		}
		const char* branchNodes = node->getDataField("branchNodes"_ts, NULL);

		std::stringstream ss(branchNodes);
		char line[64];

		const char* pathRngStart = node->getDataField("_pathRngStart"_ts, NULL);
		const char* branchNodeCount = TGE::StringTable->insert(StringMath::print(getUnitCount(branchNodes, "\t\n")), false);
		const char* sprng = TGE::Con::executef(4, "getSprng", pathRngStart, "0", branchNodeCount);

		int rng = atoi(sprng);

		int rngIndex = 0;
		while (ss.getline(line, 64, '\t')) {
			if (rngIndex == rng)
				break;
			rngIndex++;
		}

		if (isObject(line))
			return TGE::Sim::findObject(line);
		return node;
	}

	return node;
}

MBX_CONSOLE_FUNCTION(Node_getNextNode, const char*, 3, 3, "Node_getNextNode(%obj, %node)")
{
	return Node_getNextNode(argv[1], argv[2])->getIdString();
}

bool Node_isBranching(const char* nodeId) {
	TGE::SceneObject* obj = static_cast<TGE::SceneObject*>(TGE::Sim::findObject(nodeId));
	return getUnitCount(obj->getDataField("branchNodes"_ts, NULL), "\t\n");
}

MBX_CONSOLE_FUNCTION(Node_isBranching, bool, 3, 3, "Node_getNextNode(%obj, %node)")
{
	return Node_isBranching(argv[2]);
}

int Node_getPrevNode(const char* objId, const char* nodeId, const char* groupId)
{
	TGE::SimGroup* group = static_cast<TGE::SimGroup*>(TGE::Sim::findObject(groupId));
	TGE::NetObject* node = static_cast<TGE::NetObject*>(TGE::Sim::findObject(nodeId));
	for (int i = 0; i < group->mObjectList.size(); i++) {
		TGE::SimObject* obj = group->mObjectList[i];

		if (strcmp(obj->getClassRep()->getClassName(), "SimGroup") == 0) {
			int test = Node_getPrevNode(obj->getIdString(), nodeId, obj->getIdString());
			if (test != -1)
				return test;
			continue;
		}

		if (strcmp(obj->getClassRep()->getClassName(), "StaticShape") == 0) {
			TGE::GameBase* gameObj = static_cast<TGE::GameBase*>(obj);
			if (strcmp(gameObj->getDataBlock()->mName, "PathNode") == 0) {
				const char* nextNode = obj->getDataField("nextNode"_ts, NULL);
				const char* _nextNodeId = obj->getDataField("_nextNodeId"_ts, NULL);

				if ((isObject(nextNode) && TGE::Sim::findObject(nextNode)->getId() == node->getId()) || (strcmp(_nextNodeId, "") != 0 && strcmp(_nextNodeId, StringMath::print(getSyncId(node))) == 0)) {
					return obj->getId();
				}
			}
		}
	}
	return -1;
}

MBX_CONSOLE_FUNCTION(Node_getPrevNode, int, 4, 4, "Node_getPrevNode(%obj, %node, %group)")
{
	const char* arg1 = TGE::StringTable->insert(argv[1], false);
	const char* arg2 = TGE::StringTable->insert(argv[2], false);
	const char* arg3 = TGE::StringTable->insert(argv[3], false);
	return Node_getPrevNode(arg1, arg2, arg3);
}

float Node_getAdjustedProgress(const char* nodeId, float t) {
	TGE::SceneObject* obj = static_cast<TGE::SceneObject*>(TGE::Sim::findObject(nodeId));
	if (obj == NULL)
		return t;
	bool smooth = atoi(obj->getDataField("Smooth"_ts, false));
	bool smoothStart = atoi(obj->getDataField("SmoothStart"_ts, false));
	bool smoothEnd = atoi(obj->getDataField("SmoothEnd"_ts, false));

	if (smooth || (t <= 0.5 && smoothStart) || (t > 0.5 && smoothEnd))
	{
		t = -0.5 * cosf(t * 3.14159265) + 0.5;
	}

	return t;
}

MBX_CONSOLE_FUNCTION(Node_getAdjustedProgress, float, 4, 4, "Node_getAdjustedProgress(%obj, %node, %t)")
{
	float t = atof(argv[3]);
	return Node_getAdjustedProgress(argv[2], t);
}

const char* Node_getBezierHandle1(const char* nodeId) {
	TGE::SceneObject* obj = static_cast<TGE::SceneObject*>(TGE::Sim::findObject(nodeId));
	const char* bezierHandle1 = obj->getDataField("BezierHandle1"_ts, NULL);
	if (isObject(bezierHandle1))
		return bezierHandle1;

	const char* sync = TGE::Con::executef(2, "getClientSyncObject", obj->getDataField("_BezierHandle1id"_ts, NULL));

	if (strcmp(sync, "") == 0 && isObject(sync))
		return sync;

	return "-1";
}

MBX_CONSOLE_FUNCTION(Node_getBezierHandle1, const char*, 3, 3, "Node_getBezierHandle1(%obj, %node)")
{
	return Node_getBezierHandle1(argv[2]);
}

const char* Node_getBezierHandle2(const char* nodeId) {
	TGE::SceneObject* obj = static_cast<TGE::SceneObject*>(TGE::Sim::findObject(nodeId));
	const char* bezierHandle2 = obj->getDataField("BezierHandle2"_ts, NULL);
	if (isObject(bezierHandle2))
		return bezierHandle2;

	const char* sync = TGE::Con::executef(2, "getClientSyncObject", obj->getDataField("_BezierHandle2id"_ts, NULL));

	if (strcmp(sync, "") == 0 && isObject(sync))
		return sync;

	return "-1";
}

MBX_CONSOLE_FUNCTION(Node_getBezierHandle2, const char*, 3, 3, "Node_getBezierHandle2(%obj, %node)")
{
	return Node_getBezierHandle2(argv[2]);
}

std::vector<Point3F> Node_getPointList(const char* objId, const char* nodeId, const char* prevId) {

	TGE::GameBase* node = static_cast<TGE::GameBase*>(TGE::Sim::findObject(nodeId));
	TGE::GameBase* prev = static_cast<TGE::GameBase*>(TGE::Sim::findObject(prevId));

	TGE::GameBase* next = static_cast<TGE::GameBase*>(Node_getNextNode(objId, nodeId));
	TGE::GameBase* next2 = static_cast<TGE::GameBase*>(Node_getNextNode(objId, next->getIdString()));

	std::vector<Point3F> posList;

	if (next->getId() == node->getId()) {
		posList.push_back(node->getTransform().getPosition());
		return posList;
	}

	Point3F startPos = node->getTransform().getPosition();
	Point3F endPos = next->getTransform().getPosition();

	posList.push_back(startPos);

	bool nodeBezier = atoi(node->getDataField("bezier"_ts, NULL)) > 0; 
	const char* bezierHandle2 = TGE::StringTable->insert(Node_getBezierHandle2(nodeId), false);

	bool nodeSpline = atoi(node->getDataField("Spline"_ts, NULL)) > 0;

	if (nodeBezier && isObject(bezierHandle2)) {
		TGE::GameBase* bezierHandle = static_cast<TGE::GameBase*>(TGE::Sim::findObject(bezierHandle2));
		posList.push_back(bezierHandle->getTransform().getPosition());
	}
	else if (nodeSpline) {
		float nodeTime = Node_getPathTime(objId, nodeId);
		float prevTime = Node_getPathTime(objId, prevId);

		Point3F prevPos = prev->getTransform().getPosition();
		float dist = ((startPos - prevPos).len() / 3) * (fmaxf(nodeTime, 1) / fmaxf(prevTime, 1));
		Point3F sub = endPos - startPos;
		sub.normalize();

		Point3F splinePos = startPos + sub * dist;
		posList.push_back(splinePos);
	}

	bool nextBezier = atoi(next->getDataField("bezier"_ts, NULL)) > 0;
	const char* bezierHandle1 = TGE::StringTable->insert(Node_getBezierHandle1(next->getIdString()), false);

	bool nextSpline = atoi(next->getDataField("Spline"_ts, NULL)) > 0;

	if (nextBezier && isObject(bezierHandle1)) {
		TGE::GameBase* bezierHandle = static_cast<TGE::GameBase*>(TGE::Sim::findObject(bezierHandle1));
		posList.push_back(bezierHandle->getTransform().getPosition());
	}
	else if (nextSpline) {
		Point3F futurePos = next2->getTransform().getPosition();
		float dist = (endPos - startPos).len() / 3;
		Point3F sub = futurePos - startPos;
		sub.normalize();

		Point3F splinePos = endPos - sub * dist;

		posList.push_back(splinePos);
	}

	posList.push_back(endPos);

	return posList;
}

MBX_CONSOLE_FUNCTION(Node_getPointList, const char *, 4, 4, "Node_getPointList(%obj, %node, %prev)")
{
	const char* arg1 = TGE::StringTable->insert(argv[1], false);
	const char* arg2 = TGE::StringTable->insert(argv[2], false);
	const char* arg3 = TGE::StringTable->insert(argv[3], false);

	std::vector<Point3F> posList = Node_getPointList(arg1, arg2, arg3);

	std::ostringstream os;

	for (int i = 0; i < posList.size(); i++) {
		os << std::string(StringMath::print(posList[i]));
		if (i != posList.size() - 1)
			os << std::string("\t");
	}

	std::string res = os.str();
	char* retbuf = TGE::Con::getReturnBuffer(res.size() + 1);
	strcpy(retbuf, res.c_str());
	retbuf[res.size()] = '\0';

	return retbuf;
}

Point3F Node_getPathPosition(const char* objId, const char* nodeId, const char* prevId, float tVal) {
	float t = Node_getAdjustedProgress(nodeId, tVal);

	std::vector<Point3F> points = Node_getPointList(objId, nodeId, prevId);

	Point3F final = VectorBezier(t, points);

	return final;
}

MBX_CONSOLE_FUNCTION(Node_getPathPosition, const char *, 5, 5, "Node_getPathPosition(%obj, %node, %prev, %t)")
{
	const char* arg1 = TGE::StringTable->insert(argv[1], false);
	const char* arg2 = TGE::StringTable->insert(argv[2], false);
	const char* arg3 = TGE::StringTable->insert(argv[3], false);
	const char* arg4 = TGE::StringTable->insert(argv[4], false);
	return StringMath::print(Node_getPathPosition(arg1, arg2, arg3, atof(arg4)));
}

float Node_getPathTime(const char* objId, const char* nodeId) {
	TGE::SceneObject* node = static_cast<TGE::SceneObject*>(TGE::Sim::findObject(nodeId));
	float speed = atof(node->getDataField("speed"_ts, NULL));
	float delay = atof(node->getDataField("delay"_ts, NULL));

	if (speed > 0)
	{
		TGE::SceneObject* next = static_cast<TGE::SceneObject*>(Node_getNextNode(objId, nodeId));
		float distance = (next->getTransform().getPosition() - node->getTransform().getPosition()).len();
		return delay + (distance / speed) * 1000;
	}

	return delay + atof(node->getDataField("timeToNext"_ts, NULL));
}

MBX_CONSOLE_FUNCTION(Node_getPathTime, float, 3, 3, "Node_getPathTime(%obj, %node)")
{
	return Node_getPathTime(argv[1], argv[2]);
}

AngAxisF Node_getPathRotation(const char* objId, const char* nodeId, float tVal) {
	TGE::SceneObject* node = static_cast<TGE::SceneObject*>(TGE::Sim::findObject(nodeId));
	TGE::SceneObject* next = static_cast<TGE::SceneObject*>(Node_getNextNode(objId, nodeId));
	TGE::SceneObject* next2 = static_cast<TGE::SceneObject*>(Node_getNextNode(objId, next->getIdString()));

	float t = tVal;

	if (next->getId() == node->getId())
	{
		//We're the next node. So don't go anywhere.
		return AngAxisF(node->getTransform());
	}

	AngAxisF startRot = AngAxisF(node->getTransform());
	AngAxisF endRot = AngAxisF(next->getTransform());

	if (atof(node->getDataField("reverseRotation"_ts, NULL)) > 0)
	{
		t = 1 - t;
	}

	float finalT = Node_getAdjustedProgress(nodeId, t);

	QuatF finalRot = QuatF(RotInterpolate(startRot, endRot, finalT));

	//If they want it faster
	if (strcmp(node->getDataField("RotationMultiplier"_ts, NULL), "") != 0)
	{
		finalRot *= atof(node->getDataField("RotationMultiplier"_ts, NULL));
		// %rot = RotMultiply(%rot, %node.FinalRotOffset);
	}

	//Final rot applied after all other rotations
	if (strcmp(node->getDataField("FinalRotOffset"_ts, NULL), "") != 0 && strcmp(node->getDataField("FinalRotOffset"_ts, NULL), "0 0 0") != 0)
	{
		QuatF finalRotOffset = QuatF(StringMath::scan<AngAxisF>(node->getDataField("FinalRotOffset"_ts, NULL)));
		finalRot *= finalRotOffset;
		// rot = RotMultiply(%rot, %node.FinalRotOffset);
	}
	return AngAxisF(finalRot);
}

MBX_CONSOLE_FUNCTION(Node_getPathRotation, const char *, 5, 5, "Node_getPathRotation(%obj, %node, %prev, %t)")
{
	const char* arg1 = TGE::StringTable->insert(argv[1], false);
	const char* arg2 = TGE::StringTable->insert(argv[2], false);
	const char* arg3 = TGE::StringTable->insert(argv[3], false);
	const char* arg4 = TGE::StringTable->insert(argv[4], false);

	return StringMath::print(Node_getPathRotation(arg1, arg2, atof(arg4)));
}

Point3F Node_getPathScale(const char* objId, const char* nodeId, float tVal) {
	TGE::SceneObject* node = static_cast<TGE::SceneObject*>(TGE::Sim::findObject(nodeId));
	TGE::SceneObject* next = static_cast<TGE::SceneObject*>(Node_getNextNode(objId, nodeId));
	TGE::SceneObject* next2 = static_cast<TGE::SceneObject*>(Node_getNextNode(objId, next->getIdString()));

	if (next->getId() == node->getId())
	{
		//We're the next node. So don't go anywhere.
		return node->getScale();
	}

	Point3F startScale = node->getScale();
	Point3F endScale = next->getScale();

	Point3F retvalue;
	float t = Node_getAdjustedProgress(nodeId, tVal);
	retvalue.interpolate(startScale, endScale, t);

	return retvalue;
}

MBX_CONSOLE_FUNCTION(Node_getPathScale, const char *, 5, 5, "Node_getPathScale(%obj, %node, %prev, %t)")
{
	const char* arg1 = TGE::StringTable->insert(argv[1], false);
	const char* arg2 = TGE::StringTable->insert(argv[2], false);
	const char* arg3 = TGE::StringTable->insert(argv[3], false);
	const char* arg4 = TGE::StringTable->insert(argv[4], false);

	return StringMath::print(Node_getPathScale(arg1, arg2, atof(arg4)));
}

MBX_CONSOLE_FUNCTION(Node_getPathTransform, const char *, 5, 5, "Node_getPathTransform(%obj, %node, %prev, %t)")
{
	TGE::SceneObject* node = static_cast<TGE::SceneObject*>(TGE::Sim::findObject(argv[2]));
	bool usePosition = atoi(node->getDataField("usePosition"_ts, NULL)) > 0;
	bool useRotation = atoi(node->getDataField("useRotation"_ts, NULL)) > 0;
	bool useScale = atoi(node->getDataField("useScale"_ts, NULL)) > 0;

	const char* arg1 = TGE::StringTable->insert(argv[1], false);
	const char* arg2 = TGE::StringTable->insert(argv[2], false);
	const char* arg3 = TGE::StringTable->insert(argv[3], false);
	const char* arg4 = TGE::StringTable->insert(argv[4], false);

	const char* pos = TGE::StringTable->insert((usePosition ? executefnmspc("Node", "getPathPosition", 4, arg1, arg2, arg3, arg4) : ""), false);
	const char* rot = TGE::StringTable->insert((useRotation ? executefnmspc("Node", "getPathRotation", 4, arg1, arg2, arg3, arg4) : ""), false);
	const char* scale = TGE::StringTable->insert((useScale ? executefnmspc("Node", "getPathScale", 4, arg1, arg2, arg3, arg4) : ""), false);

	std::string res = std::string(pos) + "\t" + std::string(rot) + "\t" + std::string(scale);

	char* retbuf = TGE::Con::getReturnBuffer(res.size() + 1);
	strcpy(retbuf, res.c_str());
	retbuf[res.size()] = '\0';

	return retbuf;
}

MBX_CONSOLE_FUNCTION(Node_updatePath, void, 5, 5, "Node_updatePath(%obj, %node, %prev, %position)")
{
	const char* arg1 = TGE::StringTable->insert(argv[1], false);
	const char* arg2 = TGE::StringTable->insert(argv[2], false);
	const char* arg3 = TGE::StringTable->insert(argv[3], false);
	const char* arg4 = TGE::StringTable->insert(argv[4], false);

	TGE::SceneObject* node = static_cast<TGE::SceneObject*>(TGE::Sim::findObject(arg2));
	TGE::SceneObject* obj = static_cast<TGE::SceneObject*>(TGE::Sim::findObject(arg1));
	float delay = atof(node->getDataField("delay"_ts, NULL));
	float position = atof(argv[4]);

	float t;
	if (delay != 0 && position < delay) {
		t = 0;
	}
	else {
		position -= delay;
		t = mClampF(position / fmaxf(Node_getPathTime(arg1, arg2) - delay, 1), 0, 1);
	}

	bool usePosition = atoi(node->getDataField("usePosition"_ts, NULL)) > 0;
	bool useRotation = atoi(node->getDataField("useRotation"_ts, NULL)) > 0;
	bool useScale = atoi(node->getDataField("useScale"_ts, NULL)) > 0;

	Point3F pos;
	if (usePosition)
		pos = Node_getPathPosition(arg1, arg2, arg3, t);
	QuatF rot;
	if (useRotation)
		rot = QuatF(Node_getPathRotation(arg1, arg2, t));
	Point3F scale;
	if (useScale)
		scale = Node_getPathScale(arg1, arg2, t);;


	if (strcmp(obj->getClassRep()->getClassName(), "Item") == 0 && obj->isClientObject()) {
		TGE::Item* itemObj = static_cast<TGE::Item*>(obj);
		if (itemObj->mRotate()) {
			if (useRotation) {
				rot *= QuatF(Point3F(0, 0, 1), ((TGE::Sim::gCurrentTime / (float)1000) * 6.2831852) / 3);
			}
			else {
				rot = QuatF(Point3F(0, 0, 1), ((TGE::Sim::gCurrentTime / (float)1000) * 6.2831852) / 3);
			}
		}
	}

	bool update = false;
	MatrixF objTrans = obj->getTransform();

	if (usePosition) {
		objTrans.setPosition(pos);
		update = true;
	}

	if (useRotation) {
		MatrixF newMat;
		rot.setMatrix(&newMat);
		newMat.setPosition(objTrans.getPosition());
		objTrans = newMat;
		update = true;
	}

	if (update)
		obj->setTransformVirt(objTrans);

	if (useScale)
		obj->setScale(scale);
}