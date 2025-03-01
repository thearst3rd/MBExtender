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

#include <TorqueLib/core/tVector.h>
#include <TorqueLib/console/simBase.h>

namespace TGE
{
	class TextureHandle;
	class Stream;

	class MaterialList
	{
		BRIDGE_CLASS(MaterialList);
	public:
		int *something;
		U32 dunno;
		TGE::Vector<char *> mTextureNames;
		Vector<TextureHandle *> mMaterials;
		int somethingElse;
		TGE::Vector<char *> mNotSures;
		char *defaultTexture;

		CONSTRUCTOR((), 0x643c0_mac, 0x405899_win);
		MEMBERFN(bool, read, (TGE::Stream &stream), 0x63d50_mac, 0x407662_win);
	};

	class Material : public SimObject 
	{
		BRIDGE_CLASS(Material);
	public:
		U32 data_30;
		U32 data_34;
		U32 data_38;
		F32 friction;
		F32 restitution;
		F32 force;
	};

	class MaterialPropertyMap : public SimObject
	{
		BRIDGE_CLASS(MaterialPropertyMap);
	public:
		MEMBERFN(Material*, getMapEntry, (const char *name), 0x62950_mac, 0x40739c_win);
	};
}
