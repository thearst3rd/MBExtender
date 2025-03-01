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

#include <TorqueLib/gui/core/guiControl.h>

namespace TGE
{
	class TextureHandle;

	class GuiBitmapCtrl : public GuiControl
	{
		BRIDGE_CLASS(GuiBitmapCtrl);
	public:
		GETTERFN(TextureHandle *, getTextureHandle, 0x98);
		SETTERFN(TextureHandle *, setTextureHandle, 0x98);
		GETTERFN(Point2I, getStartPoint, 0x9C);
		GETTERFN(bool, getWrap, 0xA4);
		GETTERFN(const char *, getBitmap, 0x94);
		MEMBERFN(void, onRender, (Point2I offset, const RectI &updateRect), 0x408F94_win, 0x1121F0_mac);
		MEMBERFN(void, setBitmap, (const char* name, bool resize), 0x406082_win, 0x1127B0_mac);
	};
}
