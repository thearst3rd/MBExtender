#pragma once
#include "Pass.h"

class BloomPass : Pass {
public:
	BloomPass();
	virtual void processPass(Point2I& extent);
};