// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_GRAPHICS_FONT_H_
#define _SHARED_GRAPHICS_FONT_H_

#include <string>
#include "font_text.h"
#include "leak_dumper.h"

using std::string;

//class Text;

namespace Shared { namespace Graphics {
	
// =====================================================
//	class FontMetrics
// =====================================================

class FontMetrics {

private:
	float *widths;
	float height;

	//float yOffsetFactor;
	Text *textHandler;

public:
	//static float DEFAULT_Y_OFFSET_FACTOR;

	FontMetrics(Text *textHandler=NULL);
	~FontMetrics();

	//void setYOffsetFactor(float yOffsetFactor);
	//float getYOffsetFactor() const;

	void setTextHandler(Text *textHandler);
	Text * getTextHandler();

	void setWidth(int i, float width)	{this->widths[i] = width;}
	void setHeight(float height)		{this->height= height;}

	float getTextWidth(const string &str);
	float getHeight(const string &str) const;

	string wordWrapText(string text, int maxWidth);

};

// =====================================================
//	class Font
// =====================================================

class Font {
public:
	static int charCount;
	static std::string fontTypeName;
	static bool fontIsMultibyte;
	static bool forceLegacyFonts;
	static bool forceFTGLFonts;
	static bool fontIsRightToLeft;
	static float scaleFontValue;
	static float scaleFontValueCenterHFactor;
	static int baseSize;
	static int faceResolution;
	static string langHeightText;
	
public:
	enum Width {
		wNormal= 400,
		wBold= 700
	};

protected:
	string type;
	int width;
	bool inited;
	int size;
	FontMetrics metrics;
	
	Text *textHandler;

public:
	//constructor & destructor
	Font(FontTextHandlerType type);
	virtual ~Font();
	virtual void init()=0;
	virtual void end()=0;
	
	//get
	int getWidth() const;
	FontMetrics *getMetrics() 		{return &metrics;}
	Text * getTextHandler() 		{return textHandler;}
	string getType() const;

	//set
	void setType(string typeX11, string typeGeneric, string typeGenericFamily);
	void setWidth(int width);

	int getSize() const;
	void setSize(int size);

	static void bidi_cvt(string &str_);

	static void resetToDefaults();
};

// =====================================================
//	class Font2D
// =====================================================

class Font2D: public Font {

public:
	Font2D(FontTextHandlerType type=ftht_2D);
	virtual ~Font2D() {};
};

// =====================================================
//	class Font3D
// =====================================================

class Font3D: public Font {
protected:
	float depth;

public:
	Font3D(FontTextHandlerType type=ftht_3D);
	virtual ~Font3D() {};
	
	float getDepth() const			{return depth;}
	void setDepth(float depth)		{this->depth= depth;}
};

Font3D *ConvertFont2DTo3D(Font2D *font);

const char* findFont(const char *firstFontToTry=NULL,const char *firstFontFamilyToTry=NULL);

}}//end namespace

#endif
