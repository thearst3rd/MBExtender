//-----------------------------------------------------------------------------
// GuiExtension.cpp
//
// Copyright (c) 2016 The Platinum Team
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

#include <GLHelper/GLHelper.h>
#include <stdio.h>
#include <MBExtender/MBExtender.h>
#include <vector>
#include <string>
#include <map>
#include <MathLib/MathLib.h>
#include "guiMethods.hpp"
#include "../GraphicsExtension/gl.h"

#include <TorqueLib/core/stringTable.h>
#include <TorqueLib/dgl/dgl.h>
#include <TorqueLib/dgl/gFont.h>
#include <TorqueLib/gui/containers/guiWindowCtrl.h>
#include <TorqueLib/gui/controls/guiBitmapCtrl.h>
#include <TorqueLib/gui/controls/guiCheckBoxCtrl.h>
#include <TorqueLib/gui/controls/guiMLTextCtrl.h>
#include <TorqueLib/gui/core/guiTypes.h>
#include <TorqueLib/gui/core/guiCanvas.h>
#include <unordered_map>

MBX_MODULE(GuiExtension);

bool initPlugin(MBX::Plugin &plugin)
{
	GLHelper::init(plugin);

	MBX_INSTALL(plugin, GuiBorderButton);
	MBX_INSTALL(plugin, GuiExtension);
	return true;
}

std::unordered_map<TGE::GuiBitmapCtrl*, TGE::TextureObject*> specialTexs;

MBX_OVERRIDE_MEMBERFN(void, TGE::GuiBitmapCtrl::onRender, (TGE::GuiBitmapCtrl *thisptr, Point2I offset, RectI const &updateRect), originalBitmapOnRender) {

	TGE::TextureHandle *mTextureHandle = thisptr->getTextureHandle();
	TGE::TextureObject* texture = (TGE::TextureObject *) mTextureHandle;

	bool special = false;

	if (specialTexs.find(thisptr) != specialTexs.end()) {
		texture = specialTexs[thisptr];
		special = true;
	}

	RectI mBounds = thisptr->getBounds();
	TGE::GuiControlProfile *mProfile = thisptr->getProfile();

	if (mTextureHandle || special)
	{
		TGE::dglClearBitmapModulation();

		//Colorize
		const char *color = thisptr->getDataField("bitmapColor"_ts, NULL);
		ColorI bitmapColor(255, 255, 255, 255);
		if (color && strlen(color)) {
			bitmapColor = StringMath::scan<ColorI>(color);
		}

		F32 rotation = 0;
		const char *rotStr = thisptr->getDataField("bitmapRotation"_ts, NULL);
		if (rotStr && strlen(rotStr)) {
			rotation = StringMath::scan<F32>(rotStr);
		}

		if(thisptr->getWrap())
		{
			RectI srcRegion;
			RectI dstRegion;
			float xdone = ((float)mBounds.extent.x/(float)texture->mBitmapWidth)+1;
			float ydone = ((float)mBounds.extent.y/(float)texture->mBitmapHeight)+1;

			int xshift = thisptr->getStartPoint().x%texture->mBitmapWidth;
			int yshift = thisptr->getStartPoint().y%texture->mBitmapHeight;
			for(int y = 0; y < ydone; ++y)
				for(int x = 0; x < xdone; ++x)
				{
					srcRegion.set(0,0,texture->mBitmapWidth,texture->mBitmapHeight);
					dstRegion.set( ((texture->mBitmapWidth*x)+offset.x)-xshift,
								  ((texture->mBitmapHeight*y)+offset.y)-yshift,
								  texture->mBitmapWidth,
								  texture->mBitmapHeight);
					TGE::dglDrawBitmapStretchSR(texture,dstRegion, srcRegion, false);
				}
		}
		else
		{
			RectI rect(offset, mBounds.extent);
			dglDrawBitmapRotateColorStretchSR(texture, rect, RectI(0, 0, texture->mBitmapWidth, texture->mBitmapHeight), GFlip_None, rotation, bitmapColor, false);
		}
	}

	if (mProfile->getBorder() || (!mTextureHandle && !special))
	{
		RectI rect(offset.x, offset.y, mBounds.extent.x, mBounds.extent.y);
		TGE::dglDrawRect(rect, mProfile->getBorderColor());
	}

	thisptr->renderChildControls(offset, updateRect);
}

MBX_OVERRIDE_MEMBERFN(void, TGE::GuiWindowCtrl::resize, (TGE::GuiWindowCtrl *thisptr, Point2I const &pos, Point2I const &extent), originalResize) {
	//Game crashes if you call resize on an asleep GuiWindowCtrl... JOY
	if (!thisptr->isAwake()) {
		return;
	}
	originalResize(thisptr, pos, extent);
}

MBX_OVERRIDE_MEMBERFN(void, TGE::GuiCheckBoxCtrl::onRender, (TGE::GuiCheckBoxCtrl *thisptr, Point2I offset, const RectI &updateRect), originalCheckBoxRender) {
	TGE::GuiControlProfile *mProfile = thisptr->getProfile();
	RectI mBounds = thisptr->getBounds();

	ColorI backColor = thisptr->getActive() ? mProfile->getFillColor() : mProfile->getFillColorNA();
	ColorI fontColor = thisptr->getMouseOver() ? mProfile->getFontColorHL() : mProfile->getFontColor();
	ColorI insideBorderColor = thisptr->isFirstResponder() ? mProfile->getBorderColorHL() : mProfile->getBorderColor();

	const auto &rects = mProfile->bitmapArrayRects();

	// just draw the check box and the text:
	S32 xOffset = 0;

	bool checkRightAlign = StringMath::scan<bool>(thisptr->getDataField("checkRight"_ts, NULL));

	TGE::dglClearBitmapModulation();
	if(rects.size() >= 4)
	{
		S32 index = thisptr->getStateOn();
		if(!thisptr->getActive())
			index = 2;
		else if(thisptr->getDepressed())
			index += 2;
		xOffset = rects[0].extent.x + 2;
		S32 y = (mBounds.extent.y - rects[0].extent.y) / 2;

		if (checkRightAlign) {
			TGE::dglDrawBitmapSR(mProfile->getTextureHandle().object, offset + Point2I(mBounds.extent.x - xOffset, y), rects[index], false);
		} else {
			TGE::dglDrawBitmapSR(mProfile->getTextureHandle().object, offset + Point2I(0, y), rects[index], false);
		}
	}

	if(thisptr->getButtonText()[0] != 0)
	{
		TGE::dglSetBitmapModulation( fontColor );
		if (checkRightAlign) {
			thisptr->renderJustifiedText(Point2I(offset.x, offset.y),
										 Point2I(mBounds.extent.x - mBounds.extent.y - xOffset, mBounds.extent.y),
										 thisptr->getButtonText());
		} else {
			thisptr->renderJustifiedText(Point2I(offset.x + xOffset, offset.y),
										 Point2I(mBounds.extent.x - mBounds.extent.y, mBounds.extent.y),
										 thisptr->getButtonText());
		}
	}
	//render the children
	thisptr->renderChildControls(offset, updateRect);
}

// strip out any <func:> tags and call the display func on them
MBX_CONSOLE_METHOD(GuiMLTextCtrl, resolveTextFunctions, const char *, 3, 3, "%this.resolveTextFunctions(const char *text);") {
	std::string text(std::string("<color:000000>") + argv[2]);

	U32 start = 0;

	std::string defaultFont(TGE::Con::getVariable("$DefaultFont"));
	std::string defaultFontBold(TGE::Con::getVariable("$DefaultFontBold"));
	std::string defaultFontItalic(TGE::Con::getVariable("$DefaultFontItalic"));
	std::string defaultFontCondensed(TGE::Con::getVariable("$DefaultFontCondensed"));

	std::string lastFont(defaultFont);
	U32 lastFontSize = 14;

	bool canInvert = StringMath::scan<bool>(TGE::Con::getVariable("$TexturePack::InvertTextColors"))
		&& !StringMath::scan<bool>(object->getDataField("noInvert"_ts, NULL));

	while (true) {
		auto pos = text.find_first_of('<', start);
		if (pos == std::string::npos) {
			break;
		}
		auto end = text.find_first_of('>', pos + 1);
		if (end == std::string::npos) {
			break;
		}

		std::string conts = text.substr(pos + 1, end - (pos + 1));
		std::string tagName;
		std::string val = text.substr(pos, end - pos + 1);

		//Find the first part of conts
		auto colon = conts.find_first_of(':');
		if (colon == std::string::npos) {
			tagName = conts;
			conts = "";
		} else {
			tagName = conts.substr(0, colon);
			conts = conts.substr(colon + 1);
		}

		if (tagName == "func") {
			val = TGE::Con::executef(object, 2, "evalTextFunc", conts.c_str());
		} else if (tagName == "font") {
			auto nextColon = conts.find_first_of(':');

			if (nextColon == std::string::npos) {
				U32 size = StringMath::scan<U32>(conts.c_str());
				val = "<font:" + defaultFont + ":" + std::to_string(size) + ">";
				lastFont = defaultFont;
				lastFontSize = size;
			} else {
				//Font is everything before the colon
				lastFont = conts.substr(0, nextColon);
				//Size is everything after
				lastFontSize = StringMath::scan<U32>(conts.substr(nextColon + 1).c_str());

				//Special fonts, <font:bold:14> will resolve to <font:$DefaultFont["Bold"]:14>
				if (lastFont == "bold") {
					lastFont = defaultFont;
					val = "<font:" + defaultFontBold + ":" + std::to_string(lastFontSize) + ">";
				} else if (lastFont == "italic") {
					lastFont = defaultFont;
					val = "<font:" + defaultFontItalic + ":" + std::to_string(lastFontSize) + ">";
				} else if (lastFont == "condensed") {
					lastFont = defaultFont;
					val = "<font:" + defaultFontCondensed + ":" + std::to_string(lastFontSize) + ">";
				}
			}
		} else if (tagName == "bold") {
			std::string font = (lastFont == defaultFont ? defaultFontBold : lastFont + " Bold");
			U32 size = 0;
			if (conts.size() == 0) {
				size = lastFontSize;
			} else {
				//Custom size specified eg <bold:12>
				size = StringMath::scan<U32>(conts.c_str());
			}
			val = "<font:" + font + ":" + std::to_string(size) + ">";
		} else if (tagName == "italic") {
			std::string font = (lastFont == defaultFont ? defaultFontItalic : lastFont + " Italic");
			U32 size = 0;
			if (conts.size() == 0) {
				size = lastFontSize;
			} else {
				//Custom size specified eg <italic:12>
				size = StringMath::scan<U32>(conts.c_str());
			}
			val = "<font:" + font + ":" + std::to_string(size) + ">";
		} else if (tagName == "condensed") {
			std::string font = (lastFont == defaultFont ? defaultFontCondensed : lastFont + " Condensed");
			U32 size = 0;
			if (conts.size() == 0) {
				size = lastFontSize;
			} else {
				//Custom size specified eg <condensed:12>
				size = StringMath::scan<U32>(conts.c_str());
			}
			val = "<font:" + font + ":" + std::to_string(size) + ">";
		} else if (tagName == "color" && canInvert) {
			conts = TGE::Con::executef(2, "invertColor", conts.c_str());
			val = "<color:" + conts + ">";
		}

		start = val.length() + pos;
		text.replace(pos, end - pos + 1, val);
	}

	char *buffer = TGE::Con::getReturnBuffer(text.size() + 1);
	strcpy(buffer, text.c_str());

	return buffer;
}

// Returns the length of the string %text with font %font and text size
// %size. Useful for determining string lengths (not just char count).
MBX_CONSOLE_FUNCTION(textLen, int, 2, 4, "textLen(%text, %font, %size);") {
	const char *face = (argc > 2 ? argv[2] : "Arial");
	U32 size = (argc > 3 ? StringMath::scan<U32>(argv[3]) : 14);

	//The hard part: get a font
	auto pair = std::make_pair(std::string(face), size);
	TGE::Resource<TGE::GFont> font = TGE::GFont::create(face, size, TGE::sFontCacheDirectory);
	int width = font->getStrNWidth(argv[1], strlen(argv[1]));
	font.purge();

	//The easy part: get its length
	return width;
}


// misc.cs

MBX_CONSOLE_METHOD(GuiControl, setExtent, void, 3, 3, "GuiControl::setExtent(%extent)") {
	TGE::GuiControl* ctrl = static_cast<TGE::GuiControl*>(object);
	Point2F pos = StringMath::scan<Point2F>(argv[2]);
	Point2I posInt = Point2I(pos.x, pos.y);
	if (posInt != ctrl->getExtent()) {
		ctrl->resize(ctrl->getPosition(), posInt);
	}
}

MBX_CONSOLE_METHOD(GuiControl, setPosition, void, 3, 3, "GuiControl::setPosition(%position)") {
	TGE::GuiControl* ctrl = static_cast<TGE::GuiControl*>(object);
	Point2F pos = StringMath::scan<Point2F>(argv[2]);
	Point2I posInt = Point2I(pos.x, pos.y);
	if (posInt != ctrl->getPosition()) {
		ctrl->resize(posInt, ctrl->getExtent());
	}
}

MBX_CONSOLE_METHOD(GuiControl, setHeight, void, 3, 3, "GuiControl::setHeight(%height)") {
	TGE::GuiControl* ctrl = static_cast<TGE::GuiControl*>(object);
	U32 height = atoi(argv[2]);
	Point2I ctrlpos = ctrl->getPosition();
	Point2I extent = ctrl->getExtent();
	if (height != extent.y) {
		extent.y = height;
		ctrl->resize(ctrlpos, extent);
	}
}

MBX_CONSOLE_METHOD(GuiControl, setWidth, void, 3, 3, "GuiControl::setWidth(%width)") {
	TGE::GuiControl* ctrl = static_cast<TGE::GuiControl*>(object);
	U32 width = atoi(argv[2]);
	Point2I ctrlpos = ctrl->getPosition();
	Point2I extent = ctrl->getExtent();
	if (width != extent.x) {
		extent.x = width;
		ctrl->resize(ctrlpos, extent);
	}
}

MBX_CONSOLE_METHOD(GuiControl, getX, S32, 2, 2, "GuiControl::getX()") {
	TGE::GuiControl* ctrl = static_cast<TGE::GuiControl*>(object);
	return ctrl->getPosition().x;
}

MBX_CONSOLE_METHOD(GuiControl, getY, S32, 2, 2, "GuiControl::getY()") {
	TGE::GuiControl* ctrl = static_cast<TGE::GuiControl*>(object);
	return ctrl->getPosition().y;
}

MBX_CONSOLE_METHOD(GuiControl, getWidth, S32, 2, 2, "GuiControl::getWidth()") {
	TGE::GuiControl* ctrl = static_cast<TGE::GuiControl*>(object);
	return ctrl->getExtent().x;
}


MBX_CONSOLE_METHOD(GuiControl, getHeight, S32, 2, 2, "GuiControl::getHeight()") {
	TGE::GuiControl* ctrl = static_cast<TGE::GuiControl*>(object);
	return ctrl->getExtent().y;
}

MBX_CONSOLE_METHOD(GuiCanvas, getMouseControl, S32, 2, 2, "GuiCanvas::getMouseControl()") {
	TGE::GuiControl* ctrl = object->findHitControl(object->getCursorPos());
	if (ctrl != NULL) {
		return ctrl->getId();
	}
	else return 0;
}

MBX_CONSOLE_METHOD(GuiControl, isCursorOn, bool, 2, 2, "GuiControl::isCursorOn()") {
	return object->cursorInControl();
}

static const char* numPaths[] = {
	"platinum/client/ui/game/numbers/0.png",
	"platinum/client/ui/game/numbers/1.png",
	"platinum/client/ui/game/numbers/2.png",
	"platinum/client/ui/game/numbers/3.png",
	"platinum/client/ui/game/numbers/4.png",
	"platinum/client/ui/game/numbers/5.png",
	"platinum/client/ui/game/numbers/6.png",
	"platinum/client/ui/game/numbers/7.png",
	"platinum/client/ui/game/numbers/8.png",
	"platinum/client/ui/game/numbers/9.png",
	"platinum/client/ui/game/numbers/point.png",
	"platinum/client/ui/game/numbers/colon.png",
	"platinum/client/ui/game/numbers/dash.png",
	"platinum/client/ui/game/numbers/slash.png",
};

TGE::TextureObject* numHandles[14];

void unloadTimerTextures() {
	for (int i = 0; i < 14; i++) {
		if (numHandles[i] != NULL) {
			(*numHandles[i]).mRefCount--;
			numHandles[i] = NULL;
		}
	}
	specialTexs.clear();
}

MBX_CONSOLE_FUNCTION(unloadTimerTextures, void, 1, 1, "unloadTimerTextures()") {
	unloadTimerTextures();
}

MBX_CONSOLE_METHOD(GuiBitmapCtrl, setNumber, void, 3, 3, "GuiBitmapCtrl::setNumber(number)") {

	char num = argv[2][0];
	int index = -1;
	TGE::TextureObject** handle = NULL;
	if (isalpha(num)) {
		if (num == 'p') handle = &numHandles[10];
		if (num == 'c') handle = &numHandles[11];
		if (num == 'd') handle = &numHandles[12];
		if (num == 's') handle = &numHandles[13];
	}
	else {
		handle = &numHandles[num - '0'];
	}

	index = handle - &numHandles[0];

	if (index < 0 || index > 13) {
		return;
	}

	if (*handle != NULL) {
		specialTexs[object] = *handle;
	}
	else {
		*handle = TGE::TextureManager::loadTexture(numPaths[index], 0, true);
		(*handle)->mRefCount++;
		specialTexs[object] = *handle;
	}
}

MBX_CONSOLE_METHOD(GuiBitmapCtrl, setTimeNumber, void, 3, 3, "GuiBitmapCtrl::setTimeNumber(number)") {

	char num = argv[2][0];
	int index = -1;
	TGE::TextureObject** handle = NULL;
	if (isalpha(num)) {
		if (num == 'p') handle = &numHandles[10];
		if (num == 'c') handle = &numHandles[11];
		if (num == 'd') handle = &numHandles[12];
		if (num == 's') handle = &numHandles[13];
	}
	else {
		handle = &numHandles[num - '0'];
	}

	index = handle - &numHandles[0];

	if (index < 0 || index > 13) {
		return;
	}

	if (*handle != NULL) {
		specialTexs[object] = *handle;
	} else {
		*handle = TGE::TextureManager::loadTexture(numPaths[index], 0, true);
		(*handle)->mRefCount++;
		specialTexs[object] = *handle;
	}

	object->setDataField("bitmapColor"_ts, 0, TGE::Con::getVariable("$PlayTimerColor"));
}

MBX_CONSOLE_METHOD(GuiBitmapCtrl, setNumberColor, void, 4, 4, "GuiBitmapCtrl::setNumberColor(number, color)") {

	char num = argv[2][0];
	int index = -1;
	TGE::TextureObject** handle = NULL;
	if (isalpha(num)) {
		if (num == 'p') handle = &numHandles[10];
		if (num == 'c') handle = &numHandles[11];
		if (num == 'd') handle = &numHandles[12];
		if (num == 's') handle = &numHandles[13];
	}
	else {
		handle = &numHandles[num - '0'];
	}

	index = handle - &numHandles[0];

	if (index < 0 || index > 13) {
		return;
	}

	if (*handle != NULL) {
		specialTexs[object] = *handle;
	}
	else {
		*handle = TGE::TextureManager::loadTexture(numPaths[index], 0, true);
		(*handle)->mRefCount++;
		specialTexs[object] = *handle;
	}

	object->setDataField("bitmapColor"_ts, 0, TGE::StringTable->insert(argv[3], false));
}