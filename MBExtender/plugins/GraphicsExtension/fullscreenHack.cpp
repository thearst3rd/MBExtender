//-----------------------------------------------------------------------------
// fullscreenHack.cpp
//
// Copyright (c) 2015 The Platinum Team
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

#include "GraphicsExtension.h"

#include <MBExtender/MBExtender.h>

#include <TorqueLib/console/console.h>
#include <TorqueLib/game/demoGame.h>
#include <TorqueLib/platform/platformVideo.h>
#ifdef _WIN32
#include <wtypes.h>
#include <TorqueLib/platformWin32/platformWin32.h>
#include <GLHelper/GLHelper.h>
#include <GL/wglew.h>
#include <TorqueLib/gui/core/guiCanvas.h>
#define RENDERDOC
#endif

MBX_MODULE(FullscreenHack);

// Marble Blast destroys the OpenGL context for no reason at all when you tab
// out of fullscreen. Hook Video::reactivate() and Video::deactivate() and make
// them think that the game is in windowed mode.

MBX_OVERRIDE_FN(void, TGE::Video::reactivate, (bool force), originalReactivate)
{
	bool oldFullScreen = TGE::isFullScreen;
	TGE::isFullScreen = false;
	originalReactivate(false);
	TGE::isFullScreen = oldFullScreen;
}

MBX_OVERRIDE_FN(void, TGE::Video::deactivate, (bool force), originalDeactivate)
{
	bool oldFullScreen = TGE::isFullScreen;
	TGE::isFullScreen = false;
	originalDeactivate(false);
	TGE::isFullScreen = oldFullScreen;
}

#ifdef _WIN32

int oglMajor = 2;
int oglMinor = 1;

#ifdef RENDERDOC
typedef HGLRC WINAPI wglCreateContextAttribsARB_type(HDC hdc, HGLRC hShareContext,
	const int* attribList);
wglCreateContextAttribsARB_type* wglCreateContextAttribsARB_ext;

typedef BOOL WINAPI wglChoosePixelFormatARB_type(HDC hdc, const int* piAttribIList,
	const FLOAT* pfAttribFList, UINT nMaxFormats, int* piFormats, UINT* nNumFormats);
wglChoosePixelFormatARB_type* wglChoosePixelFormatARB_ext;

bool extInitialized = 0;

void init_opengl_extensions(void)
{
	// Before we can load extensions, we need a dummy OpenGL context, created using a dummy window.
	// We use a dummy window because you can only set the pixel format for a window once. For the
	// real window, we want to use wglChoosePixelFormatARB (so we can potentially specify options
	// that aren't available in PIXELFORMATDESCRIPTOR), but we can't load and use that before we
	// have a context.

	if (extInitialized)
		return;

	WNDCLASS wc;
	memset(&wc, 0, sizeof(wc));
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = DefWindowProc;
	wc.hInstance = TGE::winState.appInstance;
	wc.lpszClassName = "Dummy_WGL_djuasiodwa";	

	if (!RegisterClass(&wc)) {
		DWORD dw = GetLastError();
		LPVOID lpMsgBuf;
		FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dw,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,
			0, NULL);
		TGE::Con::errorf("Failed to register dummy OpenGL window: %s", lpMsgBuf);
	}

	HWND dummy_window = CreateWindowExA(
		0,
		wc.lpszClassName,
		"Dummy OpenGL Window",
		0,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		0,
		0,
		wc.hInstance,
		0);

	if (!dummy_window) {
		TGE::Con::errorf("Failed to create dummy OpenGL window.");
	}

	HDC dummy_dc = GetDC(dummy_window);

	PIXELFORMATDESCRIPTOR pfd;
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.cColorBits = 32;
	pfd.cAlphaBits = 8;
	pfd.iLayerType = PFD_MAIN_PLANE;
	pfd.cDepthBits = 24;
	pfd.cStencilBits = 8;

	int pixel_format = ChoosePixelFormat(dummy_dc, &pfd);
	if (!pixel_format) {
		TGE::Con::errorf("Failed to find a suitable pixel format.");
	}
	if (!SetPixelFormat(dummy_dc, pixel_format, &pfd)) {
		TGE::Con::errorf("Failed to set the pixel format.");
	}

	HGLRC dummy_context = wglCreateContext(dummy_dc);
	if (!dummy_context) {
		TGE::Con::errorf("Failed to create a dummy OpenGL rendering context.");
	}

	if (!wglMakeCurrent(dummy_dc, dummy_context)) {
		TGE::Con::errorf("Failed to activate dummy OpenGL rendering context.");
	}

	wglCreateContextAttribsARB_ext = (wglCreateContextAttribsARB_type*)wglGetProcAddress(
		"wglCreateContextAttribsARB");
	wglChoosePixelFormatARB_ext = (wglChoosePixelFormatARB_type*)wglGetProcAddress(
		"wglChoosePixelFormatARB");

	const GLubyte* oglVersion = glGetString(GL_VERSION);

	TGE::Con::printf("GLVERSION: %s", oglVersion);
	sscanf((const char*)oglVersion, "%d.%d", &oglMajor, &oglMinor);

	wglMakeCurrent(dummy_dc, 0);
	wglDeleteContext(dummy_context);
	ReleaseDC(dummy_window, dummy_dc);
	DestroyWindow(dummy_window);


	extInitialized = true;
}

#endif

GLuint renderBuffer = 0;
GLuint renderColorTexture = 0;
bool hackReady = false;
bool hackSupported = true;
LONG fullscreenWidth;
LONG fullscreenHeight;
float scaleFactor = 1;
bool glContextCreated = false;

void destroyUpscaledFramebuffer() 
{
	if (renderBuffer != NULL) 
	{
		TGE::Con::printf("Destroying fullscreen framebuffer");
		glDeleteFramebuffers(1, &renderBuffer);
		glDeleteTextures(1, &renderColorTexture);

		renderBuffer = 0;
		renderColorTexture = 0;
		GL_CheckErrors("upscalebuffer destroy");
	}
}

void generateUpscaleFramebuffer() 
{
	RECT wrect;
	GetWindowRect(TGE::winState.appWindow, &wrect);

	DEVMODE dmode;
	memset(&dmode, 0, sizeof(DEVMODE));
	EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dmode);

	fullscreenWidth = abs(wrect.right - wrect.left);
	fullscreenHeight = abs(wrect.bottom - wrect.top);
	//bool stillGoing = true;
	//int imode = 0;

	//while (stillGoing) {
	//	stillGoing = EnumDisplaySettings(NULL, imode++, &dmode);

	//	fullscreenWidth = max(fullscreenWidth, dmode.dmPelsWidth);
	//	fullscreenHeight = max(fullscreenHeight, dmode.dmPelsHeight);
	//}


	if (fullscreenWidth == 0 || fullscreenHeight == 0) {
		fullscreenWidth = TGE::winState.desktopWidth;
		fullscreenHeight = TGE::winState.desktopHeight;
	}

	scaleFactor = min((float)fullscreenWidth / (float)dmode.dmPelsWidth, (float)fullscreenHeight / (float)dmode.dmPelsHeight);

	TGE::Con::printf("Creating fullscreen framebuffer for desktop resolution %d x %d", fullscreenWidth, fullscreenHeight);
	glGenFramebuffers(1, &renderBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, renderBuffer);

	// set up non-multisampled texture
	glGenTextures(1, &renderColorTexture);
	glBindTexture(GL_TEXTURE_2D, renderColorTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fullscreenWidth, fullscreenHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderColorTexture, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	int result = (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	GL_CheckErrors("upscaleBuffer Init");

	if (result)
		hackReady = true;
	else
		hackSupported = false;

	if (!hackSupported) {
		TGE::Con::printf("PC does not support our implementation of fullscreen display scaling, falling back to previous methods");
	}
}

MBX_OVERRIDE_MEMBERFN(bool, TGE::OpenGLDevice::setScreenMode, (TGE::OpenGLDevice* thisObj, U32 width, U32 height, U32 bpp, bool fullScreen, bool forceIt, bool repaint), origSetScreenMode)
{

#ifdef RENDERDOC
	init_opengl_extensions();
#endif

	TGE::Con::printf("Desktop Size: %d x %d, Window Size: %d x %d", TGE::winState.desktopWidth, TGE::winState.desktopHeight, fullscreenWidth, fullscreenHeight);

	//width *= scaleFactor;
	//height *= scaleFactor;

	// HWND curtain = NULL;
	char errorMessage[256];
	TGE::Resolution newRes;
	newRes.size = Point2I(width, height);
	newRes.bpp = 32;
	bool newFullScreen = fullScreen;
	bool safeModeOn = TGE::Con::getBoolVariable("$pref::Video::safeModeOn");

	if (!newFullScreen && thisObj->mFullscreenOnly())
	{
		TGE::Con::warnf(TGE::ConsoleLogEntry::General, "OpenGLDevice::setScreenMode - device or desktop color depth does not allow windowed mode!");
		newFullScreen = true;
	}

	if (!newFullScreen && (newRes.size.x >= TGE::winState.desktopWidth || newRes.size.y >= TGE::winState.desktopHeight))
	{
		TGE::Con::warnf(TGE::ConsoleLogEntry::General, "OpenGLDevice::setScreenMode -- can't switch to resolution larger than desktop in windowed mode!");
		// Find the largest standard resolution that will fit on the desktop:
		U32 resIndex;
		for (resIndex = thisObj->mResolutionList().size() - 1; resIndex > 0; resIndex--)
		{
			if (thisObj->mResolutionList()[resIndex].size.x < TGE::winState.desktopWidth
				&& thisObj->mResolutionList()[resIndex].size.y < TGE::winState.desktopHeight)
				break;
		}
		newRes = thisObj->mResolutionList()[resIndex];
	}

	if (newRes.size.x < 640 || newRes.size.y < 480)
	{
		TGE::Con::warnf(TGE::ConsoleLogEntry::General, "OpenGLDevice::setScreenMode -- can't go smaller than 640x480!");
		return false;
	}

	if (newFullScreen)
	{
		if (newRes.bpp != 16 && thisObj->mFullscreenOnly())
			newRes.bpp = 16;

		// Match the new resolution to one in the list:
		U32 resIndex = 0;
		U32 bestScore = 0, thisScore = 0;
		for (int i = 0; i < thisObj->mResolutionList().size(); i++)
		{
			if (newRes.bpp == thisObj->mResolutionList()[i].bpp && newRes.size == thisObj->mResolutionList()[i].size)
			{
				resIndex = i;
				break;
			}
			else
			{
				thisScore = abs(S32(newRes.size.x) - S32(thisObj->mResolutionList()[i].size.x))
					+ abs(S32(newRes.size.y) - S32(thisObj->mResolutionList()[i].size.y))
					+ (newRes.bpp == thisObj->mResolutionList()[i].bpp ? 0 : 1);

				if (!bestScore || (thisScore < bestScore))
				{
					bestScore = thisScore;
					resIndex = i;
				}
			}
		}

		newRes = thisObj->mResolutionList()[resIndex];
	}
	else
	{
		// Basically ignore the bit depth parameter:
		newRes.bpp = TGE::winState.desktopBitsPixel;
	}

	// Return if already at this resolution:
	if (!forceIt && newRes.bpp == TGE::currentResolution.bpp && newRes.size.x == TGE::currentResolution.size.x && newRes.size.y == TGE::currentResolution.size.y && newFullScreen == TGE::isFullScreen)
		return true;

	TGE::Con::printf("Setting screen mode to %dx%dx%d (%s)...", newRes.size.x, newRes.size.y, newRes.bpp, (newFullScreen ? "fs" : "w"));

	bool needResurrect = false;

	// if ((newRes.bpp != TGE::currentResolution.bpp) || (safeModeOn && ((TGE::isFullScreen != newFullScreen) || newFullScreen)))
	// {
		//// Delete the rendering context:
		//if (TGE::winState.hGLRC)
		//{
		//	if (!TGE::videoNeedResurrect)
		//	{
		//		TGE::Con::printf("Killing the texture manager...");
		//		TGE::Game->textureKill();
		//		needResurrect = true;
		//	}

		//	TGE::Con::printf("Making the rendering context not current...");
		//	if (!wglMakeCurrent(NULL, NULL))
		//	{
		//		AssertFatal(false, "OpenGLDevice::setScreenMode\nqwglMakeCurrent( NULL, NULL ) failed!");
		//		return false;
		//	}

		//	TGE::Con::printf("Deleting the rendering context...");
		//	if (TGE::Con::getBoolVariable("$pref::Video::deleteContext") &&
		//		!wglDeleteContext(TGE::winState.hGLRC))
		//	{
		//		AssertFatal(false, "OpenGLDevice::setScreenMode\nqwglDeleteContext failed!");
		//		return false;
		//	}
		//	TGE::winState.hGLRC = NULL;
		//}

		//// Release the device context:
		//if (TGE::winState.appDC)
		//{
		//	TGE::Con::printf("Releasing the device context...");
		//	ReleaseDC(TGE::winState.appWindow, TGE::winState.appDC);
		//	TGE::winState.appDC = NULL;
		//}

		//// Destroy the window:
		//if (TGE::winState.appWindow)
		//{
		//	TGE::Con::printf("Destroying the window...");
		//	DestroyWindow(TGE::winState.appWindow);
		//	TGE::winState.appWindow = NULL;
		//}
	// }
	if (TGE::isFullScreen != newFullScreen)
	{
		// Change the window style:
		TGE::Con::printf("Changing the window style...");
		S32 windowStyle = WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
		if (newFullScreen)
			windowStyle |= (WS_POPUP | WS_MAXIMIZE);
		else
			windowStyle |= WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

		if (!SetWindowLong(TGE::winState.appWindow, GWL_STYLE, windowStyle))
			TGE::Con::errorf("SetWindowLong failed to change the window style!");
	}

	S32 test;
	if (newFullScreen)
	{
		// Change the display settings:
		DEVMODE devMode;
		memset(&devMode, 0, sizeof(devMode));
		devMode.dmSize = sizeof(devMode);
		devMode.dmPelsWidth = newRes.size.x;
		devMode.dmPelsHeight = newRes.size.y;
		devMode.dmBitsPerPel = newRes.bpp;
		devMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;

		TGE::Con::printf("Changing the display settings to %dx%dx%d...", newRes.size.x, newRes.size.y, newRes.bpp);
		// curtain = TGE::createCurtain(newRes.size.x, newRes.size.y);

		// Adjust the window rect to compensate for the window style:
		//RECT windowRect;
		//windowRect.left = windowRect.top = 0;
		//windowRect.right = TGE::winState.desktopWidth;
		//windowRect.bottom = TGE::winState.desktopHeight;

		//AdjustWindowRect(&windowRect, GetWindowLong(TGE::winState.appWindow, GWL_STYLE), false);
		//U32 adjWidth = windowRect.right - windowRect.left;
		//U32 adjHeight = windowRect.bottom - windowRect.top;

		////// Center the window on the desktop:
		//test = SetWindowPos(TGE::winState.appWindow, 0, 0, 0, adjWidth, adjHeight, SWP_NOZORDER);

		test = ChangeDisplaySettings(&devMode, CDS_FULLSCREEN); // Cause goddamnit we have to do it the old way since the gpu doesnt support it
		if (test != DISP_CHANGE_SUCCESSFUL)
		{
			TGE::isFullScreen = false;
			TGE::Con::setBoolVariable("$pref::Video::fullScreen", false);
			ChangeDisplaySettings(NULL, 0);
			TGE::Con::errorf(TGE::ConsoleLogEntry::General, "OpenGLDevice::setScreenMode - ChangeDisplaySettings failed.");
			switch (test)
			{
			case DISP_CHANGE_RESTART:
				TGE::alertOK("Display Change Failed", "You must restart your machine to get the specified mode.");
				break;

			case DISP_CHANGE_BADMODE:
				TGE::alertOK("Display Change Failed", "The specified mode is not supported by this device.");
				break;

			default:
				TGE::alertOK("Display Change Failed", "Hardware failed to switch to the specified mode.");
				break;
			};
			// DestroyWindow(curtain);
			return false;
		}
		else
			TGE::isFullScreen = true;
	}
	else if (TGE::isFullScreen)
	{
		TGE::Con::printf("Changing to the desktop display settings (%dx%dx%d)...", TGE::winState.desktopWidth, TGE::winState.desktopHeight, TGE::winState.desktopBitsPixel);
		ChangeDisplaySettings(NULL, 0);
		TGE::isFullScreen = false;
	}
	TGE::Con::setBoolVariable("$pref::Video::fullScreen", TGE::isFullScreen);

	bool newWindow = false;
	if (!TGE::winState.appWindow)
	{
		TGE::Con::printf("Creating a new %swindow...", (fullScreen ? "full-screen " : ""));
		TGE::winState.appWindow = TGE::createWindow(newRes.size.x, newRes.size.y, newFullScreen);
		if (!TGE::winState.appWindow)
		{
			AssertFatal(false, "OpenGLDevice::setScreenMode\nFailed to create a new window!");
			return false;
		}
		newWindow = true;
	}

	// Move the window:
	if (!newFullScreen)
	{
		// Adjust the window rect to compensate for the window style:
		RECT windowRect;
		windowRect.left = windowRect.top = 0;
		windowRect.right = newRes.size.x;
		windowRect.bottom = newRes.size.y;

		TGE::Con::printf("DPI: %d", GetDpiForWindow(TGE::winState.appWindow));
		AdjustWindowRectExForDpi(&windowRect, GetWindowLong(TGE::winState.appWindow, GWL_STYLE), false, 0, GetDpiForWindow(TGE::winState.appWindow));
		// AdjustWindowRect(&windowRect, GetWindowLong(TGE::winState.appWindow, GWL_STYLE), false);
		U32 adjWidth = windowRect.right - windowRect.left;
		U32 adjHeight = windowRect.bottom - windowRect.top;

		// Center the window on the desktop:
		U32 xPos = (TGE::winState.desktopWidth - adjWidth) / 2;
		U32 yPos = (TGE::winState.desktopHeight - adjHeight) / 2;
		test = SetWindowPos(TGE::winState.appWindow, 0, xPos, yPos, adjWidth, adjHeight, SWP_NOZORDER);
		if (!test)
		{
			snprintf(errorMessage, 255, "OpenGLDevice::setScreenMode\nSetWindowPos failed trying to move the window to (%d,%d) and size it to %dx%d.", xPos, yPos, newRes.size.x, newRes.size.y);
			AssertFatal(false, errorMessage);
			return false;
		}
	}
	else if (!newWindow)
	{
		// Move and size the window to take up the whole screen:
		
		if (!SetWindowPos(TGE::winState.appWindow, 0, 0, 0, newRes.size.x, newRes.size.y, SWP_NOZORDER))
		{
			snprintf(errorMessage, 255, "OpenGLDevice::setScreenMode\nSetWindowPos failed to move the window to (0,0) and size it to %dx%d.", newRes.size.x, newRes.size.y);
			AssertFatal(false, errorMessage);
			return false;
		}
	}

	bool newDeviceContext = false;
	if (!TGE::winState.appDC)
	{
		// Get a new device context:
		TGE::Con::printf("Acquiring a new device context...");
		TGE::winState.appDC = GetDC(TGE::winState.appWindow);
		if (!TGE::winState.appDC)
		{
			AssertFatal(false, "OpenGLDevice::setScreenMode\nGetDC failed to get a valid device context!");
			return false;
		}
		newDeviceContext = true;
	}

	if (newWindow)
	{
		TGE::Con::printf("Setting Pixel Format");
		// Now we can choose a pixel format the modern way, using wglChoosePixelFormatARB.
#ifdef RENDERDOC
		int pixel_format_attribs[] = {
			WGL_DRAW_TO_WINDOW_ARB,     GL_TRUE,
			WGL_SUPPORT_OPENGL_ARB,     GL_TRUE,
			WGL_DOUBLE_BUFFER_ARB,      GL_TRUE,
			WGL_ACCELERATION_ARB,       WGL_FULL_ACCELERATION_ARB,
			WGL_PIXEL_TYPE_ARB,         WGL_TYPE_RGBA_ARB,
			WGL_COLOR_BITS_ARB,         32,
			WGL_DEPTH_BITS_ARB,         24,
			WGL_STENCIL_BITS_ARB,       8,
			0
		};

		int pixel_format;
		UINT num_formats;
		wglChoosePixelFormatARB_ext(TGE::winState.appDC, pixel_format_attribs, 0, 1, &pixel_format, &num_formats);
		if (!num_formats) {
			TGE::Con::errorf("Failed to set the OpenGL 3.3 pixel format.");
		}

		PIXELFORMATDESCRIPTOR pfd;
		DescribePixelFormat(TGE::winState.appDC, pixel_format, sizeof(pfd), &pfd);
		if (!SetPixelFormat(TGE::winState.appDC, pixel_format, &pfd)) {
			TGE::Con::errorf("Failed to set the OpenGL 3.3 pixel format.");
		}
#else
		// Set the pixel format of the new window:
		PIXELFORMATDESCRIPTOR pfd;
		TGE::createPixelFormat(&pfd, newRes.bpp, 24, 8, false);
		S32 chosenFormat = TGE::chooseBestPixelFormat(TGE::winState.appDC, &pfd);
		if (!chosenFormat)
		{
			AssertFatal(false, "OpenGLDevice::setScreenMode\nNo valid pixel formats found!");
			return false;
		}
		DescribePixelFormat(TGE::winState.appDC, chosenFormat, sizeof(pfd), &pfd);
		if (!SetPixelFormat(TGE::winState.appDC, chosenFormat, &pfd))
		{
			AssertFatal(false, "OpenGLDevice::setScreenMode\nFailed to set the pixel format!");
			return false;
		}
#endif
		TGE::Con::printf("Pixel format set:");
		TGE::Con::printf("  %d color bits, %d depth bits, %d stencil bits", pfd.cColorBits, pfd.cDepthBits, pfd.cStencilBits);
	}

	if (!TGE::winState.hGLRC)
	{
		// Create a new rendering context:
		TGE::Con::printf("Creating a new rendering context...");

		int attribList[] =
		{
		  WGL_CONTEXT_MAJOR_VERSION_ARB, oglMajor,
		  WGL_CONTEXT_MINOR_VERSION_ARB, oglMinor, 
		  WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB, 0
		};
#ifdef RENDERDOC
		TGE::winState.hGLRC = wglCreateContextAttribsARB_ext(TGE::winState.appDC, NULL, attribList);
#else
		TGE::winState.hGLRC = wglCreateContext(TGE::winState.appDC);
#endif
		if (!TGE::winState.hGLRC)
		{
			AssertFatal(false, "OpenGLDevice::setScreenMode\nqwglCreateContext failed to create an OpenGL rendering context!");
			return false;
		}

		// Make the new rendering context current:
		TGE::Con::printf("Making the new rendering context current...");
		if (!wglMakeCurrent(TGE::winState.appDC, TGE::winState.hGLRC))
		{
			AssertFatal(false, "OpenGLDevice::setScreenMode\nqwglMakeCurrent failed to make the rendering context current!");
			return false;
		}

		// Just for kicks.  Seems a relatively central place to put this...
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		if (needResurrect)
		{
			// Reload the textures:
			TGE::Con::printf("Resurrecting the texture manager...");
			TGE::Game->textureResurrect();
		}
	}

	// Just for kicks.  Seems a relatively central place to put this...
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	if (newDeviceContext && TGE::glStateSuppSwapInterval)
		thisObj->setVerticalSync(!TGE::Con::getBoolVariable("$pref::Video::disableVerticalSync"));

	TGE::currentResolution = newRes;
	TGE::setWindowSize(newRes.size.x, newRes.size.y);
	char tempBuf[15];
	snprintf(tempBuf, sizeof(tempBuf), "%d %d %d", TGE::currentResolution.size.x, TGE::currentResolution.size.y, TGE::currentResolution.bpp);
	TGE::Con::setVariable("$pref::Video::resolution", tempBuf);

	// if (curtain)
	// 	DestroyWindow(curtain);

	// Doesn't hurt to do this even it isn't necessary:
	ShowWindow(TGE::winState.appWindow, SW_SHOW);
	SetForegroundWindow(TGE::winState.appWindow);
	SetFocus(TGE::winState.appWindow);

	if (repaint)
		TGE::Con::evaluatef("resetCanvas();");

	//if (glContextCreated) {
	//	destroyUpscaledFramebuffer();
	//	generateUpscaleFramebuffer();
	//}

	return true;
}

MBX_OVERRIDE_MEMBERFN(void, TGE::OpenGLDevice::swapBuffers, (TGE::OpenGLDevice* thisObj), originalBufferSwap) {
	//if (TGE::isFullScreen && hackReady) {
	//	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	//	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, renderBuffer);
	//	glBlitFramebuffer(0, 0, TGE::currentResolution.size.x, TGE::currentResolution.size.y, 0, 0, TGE::currentResolution.size.x, TGE::currentResolution.size.y, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	//	glBindFramebuffer(GL_READ_FRAMEBUFFER, renderBuffer);
	//	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	//	glBlitFramebuffer(0, 0, TGE::currentResolution.size.x, TGE::currentResolution.size.y, 0, 0, fullscreenWidth, fullscreenHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	//}
	originalBufferSwap(thisObj);
}

MBX_ON_GL_CONTEXT_DESTROY(fullscreenHackDestroy, ())
{
	// destroyUpscaledFramebuffer();
	glContextCreated = false;
}

MBX_ON_GL_CONTEXT_READY(fullscreenHackReady, ())
{
	// generateUpscaleFramebuffer();
	glContextCreated = true;
}

#endif