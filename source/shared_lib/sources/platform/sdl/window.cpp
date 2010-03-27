//This file is part of Glest Shared Library (www.glest.org)
//Copyright (C) 2005 Matthias Braun <matze@braunis.de>

//You can redistribute this code and/or modify it under
//the terms of the GNU General Public License as published by the Free Software
//Foundation; either version 2 of the License, or (at your option) any later
//version.

#include "window.h"

#include <iostream>
#include <stdexcept>
#include <cassert>
#include <cctype>

#include "conversion.h"
#include "platform_util.h"
#include "leak_dumper.h"
#include "sdl_private.h"
#include "noimpl.h"

using namespace Shared::Util;
using namespace std;

namespace Shared{ namespace Platform{

// =======================================
//               WINDOW
// =======================================

// ========== STATIC INICIALIZATIONS ==========

// Matze: hack for now...
static Window* global_window = 0;
static int oldX=0,oldY=0;
unsigned int Window::lastMouseEvent = 0;	/** for use in mouse hover calculations */
Vec2i Window::mousePos;
MouseState Window::mouseState;
bool Window::isKeyPressedDown = false;

// ========== PUBLIC ==========

Window::Window()  {
	memset(lastMouseDown, 0, sizeof(lastMouseDown));

	assert(global_window == 0);
	global_window = this;

	lastMouseEvent = 0;
	mousePos = Vec2i(0);
	mouseState.clear();
}

Window::~Window() {
	assert(global_window == this);
	global_window = 0;
}

bool Window::handleEvent() {
	SDL_Event event;
	SDL_GetMouseState(&oldX,&oldY);

	while(SDL_PollEvent(&event)) {
		try {
		    //printf("START [%d]\n",event.type);

            switch(event.type) {
                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP:
                case SDL_MOUSEMOTION:

                    //printf("In [%s::%s] Line :%d\n",__FILE__,__FUNCTION__,__LINE__);
               		setLastMouseEvent(Chrono::getCurMillis());
               		setMousePos(Vec2i(event.button.x, event.button.y));
                    break;
            }

			switch(event.type) {
				case SDL_QUIT:
                    //printf("In [%s::%s] Line :%d\n",__FILE__,__FUNCTION__,__LINE__);
					return false;
				case SDL_MOUSEBUTTONDOWN:
                    //printf("In [%s::%s] Line :%d\n",__FILE__,__FUNCTION__,__LINE__);
                    if(global_window) {
                        global_window->handleMouseDown(event);
                    }
					break;
				case SDL_MOUSEBUTTONUP: {
				    //printf("In [%s::%s] Line :%d\n",__FILE__,__FUNCTION__,__LINE__);
				    if(global_window) {
                        MouseButton b = getMouseButton(event.button.button);
                        setMouseState(b, false);

                        global_window->eventMouseUp(event.button.x,
							event.button.y,getMouseButton(event.button.button));
				    }
					break;
				}
				case SDL_MOUSEMOTION: {
				    //printf("In [%s::%s] Line :%d\n",__FILE__,__FUNCTION__,__LINE__);
					//MouseState ms;
					//ms.leftMouse = (event.motion.state & SDL_BUTTON_LMASK) != 0;
					//ms.rightMouse = (event.motion.state & SDL_BUTTON_RMASK) != 0;
					//ms.centerMouse = (event.motion.state & SDL_BUTTON_MMASK) != 0;
                    setMouseState(mbLeft, event.motion.state & SDL_BUTTON_LMASK);
                    setMouseState(mbRight, event.motion.state & SDL_BUTTON_RMASK);
                    setMouseState(mbCenter, event.motion.state & SDL_BUTTON_MMASK);

					if(global_window) {
						global_window->eventMouseMove(event.motion.x, event.motion.y, &getMouseState()); //&ms);
					}
					break;
				}
				case SDL_KEYDOWN:
                    //printf("In [%s::%s] Line :%d\n",__FILE__,__FUNCTION__,__LINE__);

					Window::isKeyPressedDown = true;
					/* handle ALT+Return */
					if(event.key.keysym.sym == SDLK_RETURN
							&& (event.key.keysym.mod & (KMOD_LALT | KMOD_RALT))) {
						toggleFullscreen();
					}
					if(global_window) {
                        global_window->eventKeyDown(getKey(event.key.keysym));
                        global_window->eventKeyPress(static_cast<char>(event.key.keysym.unicode));
					}
					break;
				case SDL_KEYUP:
                    //printf("In [%s::%s] Line :%d\n",__FILE__,__FUNCTION__,__LINE__);

					Window::isKeyPressedDown = false;

                    if(global_window) {
                        global_window->eventKeyUp(getKey(event.key.keysym));
                    }
					break;
			}
		} catch(std::exception& e) {
			std::cerr << "Couldn't process event: " << e.what() << "\n";
		}
	}

    //printf("END [%d]\n",event.type);

	return true;
}

void Window::revertMousePos() {
	SDL_WarpMouse(oldX, oldY);
}

string Window::getText() {
	char* c = 0;
	SDL_WM_GetCaption(&c, 0);

	return string(c);
}

float Window::getAspect() {
	return static_cast<float>(getClientH())/getClientW();
}

void Window::setText(string text) {
	SDL_WM_SetCaption(text.c_str(), 0);
}

void Window::setSize(int w, int h) {
	this->w = w;
	this->h = h;
	Private::ScreenWidth = w;
	Private::ScreenHeight = h;
}

void Window::setPos(int x, int y)  {
	if(x != 0 || y != 0) {
		NOIMPL;
		return;
	}
}

void Window::minimize() {
	NOIMPL;
}

void Window::setEnabled(bool enabled) {
	NOIMPL;
}

void Window::setVisible(bool visible) {
	 NOIMPL;
}

void Window::setStyle(WindowStyle windowStyle) {
	if(windowStyle == wsFullscreen)
		return;
	// NOIMPL;
}

void Window::create() {
	// nothing here
}

void Window::destroy() {
	SDL_Event event;
	event.type = SDL_QUIT;
	SDL_PushEvent(&event);
}

void Window::toggleFullscreen() {
	SDL_WM_ToggleFullScreen(SDL_GetVideoSurface());
}

void Window::handleMouseDown(SDL_Event event) {
	static const Uint32 DOUBLECLICKTIME = 500;
	static const int DOUBLECLICKDELTA = 5;

	MouseButton button = getMouseButton(event.button.button);

	// windows implementation uses 120 for the resolution of a standard mouse
	// wheel notch.  However, newer mice have finer resolutions.  I dunno if SDL
	// handles those, but for now we're going to say that each mouse wheel
	// movement is 120.
	if(button == mbWheelUp) {
	    //printf("button == mbWheelUp\n");
		eventMouseWheel(event.button.x, event.button.y, 120);
		return;
	} else if(button == mbWheelDown) {
	    //printf("button == mbWheelDown\n");
		eventMouseWheel(event.button.x, event.button.y, -120);
		return;
	}


	Uint32 ticks = SDL_GetTicks();
	int n = (int) button;
	if(ticks - lastMouseDown[n] < DOUBLECLICKTIME
			&& abs(lastMouseX[n] - event.button.x) < DOUBLECLICKDELTA
			&& abs(lastMouseY[n] - event.button.y) < DOUBLECLICKDELTA) {
		eventMouseDown(event.button.x, event.button.y, button);
		eventMouseDoubleClick(event.button.x, event.button.y, button);
	} else {
		eventMouseDown(event.button.x, event.button.y, button);
	}
	lastMouseDown[n] = ticks;
	lastMouseX[n] = event.button.x;
	lastMouseY[n] = event.button.y;
}

MouseButton Window::getMouseButton(int sdlButton) {
	switch(sdlButton) {
		case SDL_BUTTON_LEFT:
			return mbLeft;
		case SDL_BUTTON_RIGHT:
			return mbRight;
		case SDL_BUTTON_MIDDLE:
			return mbCenter;
        case SDL_BUTTON_WHEELUP:
            return mbWheelUp;
        case SDL_BUTTON_WHEELDOWN:
            return mbWheelDown;
		default:
			throw std::runtime_error("Mouse Button > 3 not handled.");
	}
}

char Window::getKey(SDL_keysym keysym) {
	switch(keysym.sym) {
		case SDLK_PLUS:
		case SDLK_KP_PLUS:
			return vkAdd;
		case SDLK_MINUS:
		case SDLK_KP_MINUS:
			return vkSubtract;
		case SDLK_LALT:
		case SDLK_RALT:
			return vkAlt;
		case SDLK_LCTRL:
		case SDLK_RCTRL:
			return vkControl;
		case SDLK_LSHIFT:
		case SDLK_RSHIFT:
			return vkShift;
		case SDLK_ESCAPE:
			return vkEscape;
		case SDLK_UP:
			return vkUp;
		case SDLK_LEFT:
			return vkLeft;
		case SDLK_RIGHT:
			return vkRight;
		case SDLK_DOWN:
			return vkDown;
		case SDLK_RETURN:
		case SDLK_KP_ENTER:
			return vkReturn;
		case SDLK_BACKSPACE:
			return vkBack;
		case SDLK_0:
			return '0';
		case SDLK_1:
			return '1';
		case SDLK_2:
			return '2';
		case SDLK_3:
			return '3';
		case SDLK_4:
			return '4';
		case SDLK_5:
			return '5';
		case SDLK_6:
			return '6';
		case SDLK_7:
			return '7';
		case SDLK_8:
			return '8';
		case SDLK_9:
			return '9';
		case SDLK_a:
			return 'A';
		case SDLK_b:
			return 'B';
		case SDLK_c:
			return 'C';
		case SDLK_d:
			return 'D';
		case SDLK_e:
			return 'E';
		case SDLK_f:
			return 'F';
		case SDLK_g:
			return 'G';
		case SDLK_h:
			return 'H';
		case SDLK_i:
			return 'I';
		case SDLK_j:
			return 'J';
		case SDLK_k:
			return 'K';
		case SDLK_l:
			return 'L';
		case SDLK_m:
			return 'M';
		case SDLK_n:
			return 'N';
		case SDLK_o:
			return 'O';
		case SDLK_p:
			return 'P';
		case SDLK_q:
			return 'Q';
		case SDLK_r:
			return 'R';
		case SDLK_s:
			return 'S';
		case SDLK_t:
			return 'T';
		case SDLK_u:
			return 'U';
		case SDLK_v:
			return 'V';
		case SDLK_w:
			return 'W';
		case SDLK_x:
			return 'X';
		case SDLK_y:
			return 'Y';
		case SDLK_z:
			return 'Z';
		default:
			Uint16 c = keysym.unicode;
			if((c & 0xFF80) == 0) {
				return toupper(c);
			}
			break;
	}

	return 0;
}

}}//end namespace
