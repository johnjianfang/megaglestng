// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================
#ifndef _SHARED_PLATFORM_WINDOW_H_
#define _SHARED_PLATFORM_WINDOW_H_

#include <map>
#include <string>
#include <cassert>

#include "types.h"
#include "vec.h"
#include "platform_menu.h"

using std::map;
using std::string;
using Shared::Graphics::Vec2i;

namespace Shared{ namespace Platform{

class Timer;
class PlatformContextGl;

enum MouseButton{
	mbUnknown,
	mbLeft,
	mbCenter,
	mbRight,
	mbWheelUp,
	mbWheelDown,
	mbButtonX1,
	mbButtonX2,

	mbCount
};

enum SizeState{
	ssMaximized,
	ssMinimized,
	ssRestored
};

const int vkAdd= VK_ADD;
const int vkSubtract= VK_SUBTRACT;
const int vkAlt= VK_MENU;
const int vkControl= VK_CONTROL;
const int vkShift= VK_SHIFT;
const int vkEscape= VK_ESCAPE;
const int vkUp= VK_UP;
const int vkLeft= VK_LEFT;
const int vkRight= VK_RIGHT;
const int vkDown= VK_DOWN;
const int vkReturn= VK_RETURN;
const int vkBack= VK_BACK;
const int vkDelete= VK_DELETE;
const int vkF1= VK_F1;

class MouseState {
private:
	bool states[mbCount];


public:
	MouseState() {
		clear();
	}
	//MouseState(const MouseState &);
	//MouseState &operator=(const MouseState &);
	void clear() { memset(this, 0, sizeof(MouseState)); }

	bool get(MouseButton b) const {
		assert(b > 0 && b < mbCount);
		return states[b];
	}

	void set(MouseButton b, bool state) {
		assert(b > 0 && b < mbCount);
		states[b] = state;
	}
};

enum WindowStyle{
	wsFullscreen,
	wsWindowedFixed,
	wsWindowedResizeable
};

// =====================================================
//	class Window
// =====================================================

class Window{
private:
	typedef map<WindowHandle, Window*> WindowMap;

private:
	static const DWORD fullscreenStyle;
	static const DWORD windowedFixedStyle;
	static const DWORD windowedResizeableStyle;

	static int nextClassName;
	static WindowMap createdWindows;

    static unsigned int lastMouseEvent;	/** for use in mouse hover calculations */
    static MouseState mouseState;
    static Vec2i mousePos;
    static bool isKeyPressedDown;

    static void setLastMouseEvent(unsigned int lastMouseEvent)	{Window::lastMouseEvent = lastMouseEvent;}
    static unsigned int getLastMouseEvent() 				    {return Window::lastMouseEvent;}

    static const MouseState &getMouseState() 				    {return Window::mouseState;}
    static void setMouseState(MouseButton b, bool state)		{Window::mouseState.set(b, state);}

    static const Vec2i &getMousePos() 					        {return Window::mousePos;}
    static void setMousePos(const Vec2i &mousePos)				{Window::mousePos = mousePos;}

protected:
	WindowHandle handle;
	WindowStyle windowStyle;
	string text;
	int x;
	int y;
	int w;
	int h;
	string className;
	DWORD style;
	DWORD exStyle;
	bool ownDc;

public:
	static bool handleEvent();
	static void revertMousePos();
	static bool isKeyDown() { return isKeyPressedDown; }

	//contructor & destructor
	Window();
	virtual ~Window();

	WindowHandle getHandle()	{return handle;}
	string getText();
	int getX()					{return x;}
	int getY()					{return y;}
	int getW()					{return w;}
	int getH()					{return h;}

	//component state
	int getClientW();
	int getClientH();
	float getAspect();

	//object state
	void setText(string text);
	void setStyle(WindowStyle windowStyle);
	void setSize(int w, int h);
	void setPos(int x, int y);
	void setEnabled(bool enabled);
	void setVisible(bool visible);

	//misc
	void create();
	void minimize();
	void maximize();
	void restore();
	void showPopupMenu(Menu *menu, int x, int y);
	void destroy();

protected:
	virtual void eventCreate(){}
	virtual void eventMouseDown(int x, int y, MouseButton mouseButton){}
	virtual void eventMouseUp(int x, int y, MouseButton mouseButton){}
	virtual void eventMouseMove(int x, int y, const MouseState *mouseState){}
	virtual void eventMouseDoubleClick(int x, int y, MouseButton mouseButton){}
	virtual void eventKeyDown(char key){}
	virtual void eventMouseWheel(int x, int y, int zDelta){}
	virtual void eventKeyUp(char key){}
	virtual void eventKeyPress(char c){};
	virtual void eventResize(){};
	virtual void eventPaint(){}
	virtual void eventActivate(bool activated){};
	virtual void eventResize(SizeState sizeState){};
	virtual void eventMenu(int menuId){}
	virtual void eventClose(){};
	virtual void eventDestroy(){};

private:
	static LRESULT CALLBACK eventRouter(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static int getNextClassName();
	void registerWindow(WNDPROC wndProc= NULL);
	void createWindow(LPVOID creationData= NULL);
	void mouseyVent(int asdf, MouseButton mouseButton) {
		const Vec2i &mousePos = getMousePos();
		switch(asdf) {
		case 0:
			setMouseState(mouseButton, true);
			eventMouseDown(mousePos.x, mousePos.y, mouseButton);
			break;
		case 1:
			setMouseState(mouseButton, false);
			eventMouseUp(mousePos.x, mousePos.y, mouseButton);
			break;
		case 2:
			eventMouseDoubleClick(mousePos.x, mousePos.y, mouseButton);
			break;
		}
	}

};

}}//end namespace

#endif
