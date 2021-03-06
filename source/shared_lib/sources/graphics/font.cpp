// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2007 Martio Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "font.h"
#include <stdexcept>
#include <string.h>
#include "conversion.h"

#ifdef USE_FTGL
#include "font_textFTGL.h"
#include <vector>
#include <algorithm>
using namespace Shared::Graphics::Gl;
#endif

#ifdef USE_FREETYPEGL
#include "font_text_freetypegl.h"
#endif

// If your compiler has a version older than 2.4.1 of fontconfig, you can tell cmake to
// disable trying to use fontconfig via passing this to cmake:
// -DWANT_FONTCONFIG=Off
#ifdef HAVE_FONTCONFIG
#include <fontconfig/fontconfig.h>
#endif

#include "util.h"
#include "platform_common.h"
#include "platform_util.h"

#ifdef	HAVE_FRIBIDI
#include <fribidi.h>
#endif

#include "leak_dumper.h"

using namespace std;
using namespace Shared::Util;
using namespace Shared::PlatformCommon;

namespace Shared { namespace Graphics {

// Init statics
int Font::charCount							= 256;
std::string Font::fontTypeName 				= "Times New Roman";
bool Font::fontIsMultibyte 					= false;
bool Font::forceLegacyFonts					= false;
bool Font::fontIsRightToLeft				= false;
bool Font::forceFTGLFonts					= false;

// This value is used to scale the font text rendering
// in 3D render mode
float Font::scaleFontValue					= 0.80f;
// This value is used for centering font text vertically (height)
float Font::scaleFontValueCenterHFactor		= 4.0f;
//float Font::scaleFontValue					= 1.0;
//float Font::scaleFontValueCenterHFactor		= 4.0;

int Font::baseSize							= 3;

int Font::faceResolution					= 72;
string Font::langHeightText					= "yW";
//

void Font::resetToDefaults() {
	Font::charCount					= 256;
	Font::fontTypeName 				= "Times New Roman";
	Font::fontIsMultibyte 			= false;
	//Font::forceLegacyFonts			= false;
	Font::fontIsRightToLeft			= false;

	// This value is used to scale the font text rendering
	// in 3D render mode
	Font::scaleFontValue					= 0.80f;
	// This value is used for centering font text vertically (height)
	Font::scaleFontValueCenterHFactor		= 4.0f;
	//float Font::scaleFontValue					= 1.0;
	//float Font::scaleFontValueCenterHFactor		= 4.0;

	Font::baseSize							= 3;

	Font::faceResolution					= 72;
	Font::langHeightText					= "yW";

#if defined(WIN32)
	string newEnvValue = "MEGAGLEST_FONT=";
	_putenv(newEnvValue.c_str());
	newEnvValue = "MEGAGLEST_FONT_FAMILY=";
	_putenv(newEnvValue.c_str());
#else
	unsetenv("MEGAGLEST_FONT");
	unsetenv("MEGAGLEST_FONT_FAMILY");
#endif
}

// =====================================================
//	class FontMetrics
// =====================================================

FontMetrics::FontMetrics(Text *textHandler) {
	this->textHandler 	= textHandler;
	this->widths		= new float[Font::charCount];
	this->height		= 0;

	for(int i=0; i < Font::charCount; ++i) {
		widths[i]= 0;
	}
}

FontMetrics::~FontMetrics() {
	delete [] widths;
	widths = NULL;
}

void FontMetrics::setTextHandler(Text *textHandler) {
	this->textHandler = textHandler;
}

Text * FontMetrics::getTextHandler() {
	return this->textHandler;
}

float FontMetrics::getTextWidth(const string &str) {
	string longestLine = "";
	size_t found = str.find("\n");
	if (found == string::npos) {
		longestLine = str;
	}
	else {
		vector<string> lineTokens;
		Tokenize(str,lineTokens,"\n");
		if(lineTokens.empty() == false) {
			for(unsigned int i = 0; i < lineTokens.size(); ++i) {
				string currentStr = lineTokens[i];
				if(currentStr.length() > longestLine.length()) {
					longestLine = currentStr;
				}
			}
		}
    }

	if(textHandler != NULL) {
		//printf("str [%s] textHandler->Advance = %f Font::scaleFontValue = %f\n",str.c_str(),textHandler->Advance(str.c_str()),Font::scaleFontValue);
		return (textHandler->Advance(longestLine.c_str()) * Font::scaleFontValue);
		//return (textHandler->Advance(str.c_str()));
	}
	else {
		float width= 0.f;
		for(unsigned int i=0; i< longestLine.size() && (int)i < Font::charCount; ++i){
			if(longestLine[i] >= Font::charCount) {
				string sError = "str[i] >= Font::charCount, [" + longestLine + "] i = " + uIntToStr(i);
				throw megaglest_runtime_error(sError);
			}
			//Treat 2 byte characters as spaces
			if(longestLine[i] < 0) {
				width+= (widths[97]); // This is the letter a which is a normal wide character and good to use for spacing
			}
			else {
				width+= widths[longestLine[i]];
			}
		}
		return width;
	}
}

float FontMetrics::getHeight(const string &str) const {
	if(textHandler != NULL) {
		//printf("(textHandler->LineHeight(" ") = %f Font::scaleFontValue = %f\n",textHandler->LineHeight(" "),Font::scaleFontValue);
		//return (textHandler->LineHeight(str.c_str()) * Font::scaleFontValue);
		return (textHandler->LineHeight(str.c_str()));
	}
	else {
		return height;
	}
}

string FontMetrics::wordWrapText(string text, int maxWidth) {
	// Strip newlines from source
    replaceAll(text, "\n", " \n ");

	// Get all words (space separated text)
	vector<string> words;
	Tokenize(text,words," ");

	string wrappedText = "";
    float lineWidth = 0.0f;

    for(unsigned int i = 0; i < words.size(); ++i) {
    	string word = words[i];
    	if(word=="\n"){
    		wrappedText += word;
    		lineWidth = 0;
    	}
    	else {
    		float wordWidth = this->getTextWidth(word);
    		if (lineWidth + wordWidth > maxWidth) {
    			wrappedText += "\n";
    			lineWidth = 0;
    		}
    		lineWidth += this->getTextWidth(word+" ");
    		wrappedText += word + " ";
    	}
    }

    return wrappedText;
}

// ===============================================
//	class Font
// ===============================================

Font::Font(FontTextHandlerType type) {
	assert(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false);

	inited		= false;
	this->type	= fontTypeName;
	width		= 400;
	size 		= 10;
	textHandler = NULL;

#if defined(USE_FTGL) || defined(USE_FREETYPEGL)

	if(Font::forceLegacyFonts == false) {
		try {
#if defined(USE_FREETYPEGL)
			//if(Font::forceFTGLFonts == false) {
			//	textHandler = NULL;
			//	textHandler = new TextFreetypeGL(type);
				//printf("TextFreetypeGL is ON\n");
			//}

			//else
#endif
			{
				TextFTGL::faceResolution = Font::faceResolution;
				TextFTGL::langHeightText = Font::langHeightText;

				textHandler = NULL;
				textHandler = new TextFTGL(type);
				//printf("TextFTGL is ON\n");

				TextFTGL::faceResolution = Font::faceResolution;
				TextFTGL::langHeightText = Font::langHeightText;
			}

			metrics.setTextHandler(this->textHandler);
		}
		catch(exception &ex) {
			SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
			textHandler = NULL;
		}
	}

#endif
}

Font::~Font() {
	if(textHandler) {
		delete textHandler;
	}
	textHandler = NULL;
}

string Font::getType() const {
	return this->type;
}

void Font::setType(string typeX11, string typeGeneric, string typeGenericFamily) {
	if(textHandler) {
		try {
			this->type= typeGeneric;
			textHandler->init(typeGeneric,typeGenericFamily,textHandler->GetFaceSize());
			metrics.setTextHandler(this->textHandler);
		}
		catch(exception &ex) {
			SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
			textHandler = NULL;
		}
	}
	if(textHandler == NULL) {
		this->type= typeX11;
	}
}

void Font::setWidth(int width) {
	this->width= width;
}

int Font::getWidth() const	{
	return width;
}

int Font::getSize() const	{
	if(textHandler) {
		return textHandler->GetFaceSize();
	}
	else {
		return size;
	}
}
void Font::setSize(int size)	{
	if(textHandler) {
		return textHandler->SetFaceSize(size);
	}
	else {
		this->size= size;
	}
}

void Font::bidi_cvt(string &str_) {
#ifdef	HAVE_FRIBIDI
	char		*c_str = const_cast<char *>(str_.c_str());	// fribidi forgot const...
	FriBidiStrIndex	len = str_.length();
	FriBidiChar	*bidi_logical = new FriBidiChar[len + 2];
	FriBidiChar	*bidi_visual = new FriBidiChar[len + 2];
	char		*utf8str = new char[4*len + 1];	//assume worst case here (all 4 Byte characters)
	FriBidiCharType	base_dir = FRIBIDI_TYPE_ON;
	FriBidiStrIndex n;


#ifdef	OLD_FRIBIDI
	n = fribidi_utf8_to_unicode (c_str, len, bidi_logical);
#else
	n = fribidi_charset_to_unicode(FRIBIDI_CHAR_SET_UTF8, c_str, len, bidi_logical);
#endif
	fribidi_log2vis(bidi_logical, n, &base_dir, bidi_visual, NULL, NULL, NULL);
#ifdef	OLD_FRIBIDI
	fribidi_unicode_to_utf8 (bidi_visual, n, utf8str);
#else
	fribidi_unicode_to_charset(FRIBIDI_CHAR_SET_UTF8, bidi_visual, n, utf8str);
#endif
	//is_rtl_ = base_dir == FRIBIDI_TYPE_RTL;
	//fontIsRightToLeft = base_dir == FRIBIDI_TYPE_RTL;
	fontIsRightToLeft = false;

	str_ = std::string(utf8str);
	delete[] bidi_logical;
	delete[] bidi_visual;
	delete[] utf8str;
#endif
}

// ===============================================
//	class Font2D
// ===============================================

Font2D::Font2D(FontTextHandlerType type) : Font(type) {
}

// ===============================================
//	class Font3D
// ===============================================

Font3D::Font3D(FontTextHandlerType type) : Font(type) {
	depth= 10.f;
}

string findFontFamily(const char* font, const char *fontFamily) {
	string resultFile = "";

	// If your compiler has a version older than 2.4.1 of fontconfig, you can tell cmake to
	// disable trying to use fontconfig via passing this to cmake:
	// -DWANT_FONTCONFIG=Off

#ifdef HAVE_FONTCONFIG
	// Get default font via fontconfig
	if( !font && FcInit() && fontFamily)	{
		FcResult result;
		FcFontSet *fs;
		FcPattern* pat;
		FcPattern *match;

		/*
		TRANSLATORS: If using the FTGL backend, this should be the font
		name of a font that contains all the Unicode characters in use in
		your translation.
		*/
		//pat = FcNameParse((FcChar8 *)"Gothic Uralic");
		pat = FcNameParse((FcChar8 *)fontFamily);
		FcConfigSubstitute(0, pat, FcMatchPattern);

		//FcPatternDel(pat, FC_WEIGHT);
		//FcPatternAddInteger(pat, FC_WEIGHT, FC_WEIGHT_BOLD);

		FcDefaultSubstitute(pat);
		fs = FcFontSetCreate();
		match = FcFontMatch(0, pat, &result);

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Trying fontconfig for fontfamily [%s]\n",(fontFamily != NULL ? fontFamily : "null"));

		if (match) FcFontSetAdd(fs, match);
		if (pat) FcPatternDestroy(pat);
		if(fs) {
			FcChar8* file;
			if( FcPatternGetString (fs->fonts[0], FC_FILE, 0, &file) == FcResultMatch ) {
				//CHECK_FONT_PATH((const char*)file,NULL)
				resultFile = (const char*)file;
			}
			FcFontSetDestroy(fs);
		}
		// If your compiler has a version older than 2.4.1 of fontconfig, you can tell cmake to
		// disable trying to use fontconfig via passing this to cmake:
		// -DWANT_FONTCONFIG=Off
		FcFini();
	}
	else {
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("******************* FONT CONFIG will not be called font [%s] fontFamily [%s]!\n",(font != NULL ? font : "null"),(fontFamily != NULL ? fontFamily : "null"));
	}
#else
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("******************* NO FONT CONFIG ENABLED!\n");
#endif

	return resultFile;
}

const char* findFont(const char *firstFontToTry,const char *firstFontFamilyToTry) {
	const char* font = NULL;
	const char* path = NULL;

	#define CHECK_FONT_PATH(filename,fontFamily) \
	{ \
		path = filename; \
		if( font == NULL && path != NULL && strlen(path) > 0 && fileExists(path) == true ) { \
			font = strdup(path); \
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("#1 candidate font file exists [%s]\n",(font != NULL ? font : "null")); \
		} \
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("#1 Searching for font file [%s] result [%s]\n",(path != NULL ? path : "null"),(font != NULL ? font : "null")); \
		if( font == NULL && fontFamily != NULL && strlen(fontFamily) > 0) { \
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("#2 Searching for font [%s] family [%s]\n",(font != NULL ? font : "null"),(fontFamily != NULL ? fontFamily : "null")); \
			string fileFound = findFontFamily(font, fontFamily); \
			if(fileFound != "") { \
				path = fileFound.c_str(); \
				if(SystemFlags::VERBOSE_MODE_ENABLED) printf("#2 candidate font file found [%s]\n",fileFound.c_str()); \
				if( path != NULL && strlen(path) > 0 && fileExists(path) == true ) { \
					if(font) free((void*)font); \
					font = strdup(path); \
					if(SystemFlags::VERBOSE_MODE_ENABLED) printf("#2 candidate font file has been set[%s]\n",(font != NULL ? font : "null")); \
				} \
			} \
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("#2 Searching for font family [%s] result [%s]\n",(fontFamily != NULL ? fontFamily : "null"),(font != NULL ? font : "null")); \
		} \
	}

	string tryFont = "";
	if(firstFontToTry != NULL || firstFontFamilyToTry != NULL) {
		if(firstFontToTry != NULL && strlen(firstFontToTry) > 0) {
			tryFont = firstFontToTry;
			#ifdef WIN32
			  replaceAll(tryFont, "/", "\\");
			#endif


			CHECK_FONT_PATH(tryFont.c_str(),firstFontFamilyToTry)
		}
		else {
			CHECK_FONT_PATH(NULL,firstFontFamilyToTry)
		}
	}

	// Get user-specified font path
	if(getenv("MEGAGLEST_FONT") != NULL || getenv("MEGAGLEST_FONT_FAMILY") != NULL) {

		if(getenv("MEGAGLEST_FONT") != NULL) {
			tryFont = getenv("MEGAGLEST_FONT");

			if(Text::DEFAULT_FONT_PATH_ABSOLUTE != "") {
				tryFont = Text::DEFAULT_FONT_PATH_ABSOLUTE + "/" + extractFileFromDirectoryPath(tryFont);
			}
			#ifdef WIN32
			  replaceAll(tryFont, "/", "\\");
			#endif

			CHECK_FONT_PATH(tryFont.c_str(),getenv("MEGAGLEST_FONT_FAMILY"))
		}
		else {
			CHECK_FONT_PATH(NULL,getenv("MEGAGLEST_FONT_FAMILY"))
		}
	}

	string data_path = Text::DEFAULT_FONT_PATH;
	string defaultFont = data_path + "data/core/fonts/LinBiolinum_RB.ttf";//LinBiolinum_Re-0.6.4.ttf
	if(Text::DEFAULT_FONT_PATH_ABSOLUTE != "") {
		data_path = Text::DEFAULT_FONT_PATH;
		defaultFont = data_path + "/LinBiolinum_RB.ttf";//LinBiolinum_Re-0.6.4.ttf
	}
	tryFont = defaultFont;
	#ifdef WIN32
	  replaceAll(tryFont, "/", "\\");
	#endif
	CHECK_FONT_PATH(tryFont.c_str(),"Linux Biolinum O:style=Bold")

	#ifdef FONT_PATH
	// Get distro-specified font path
	CHECK_FONT_PATH(FONT_PATH)
	#endif

	CHECK_FONT_PATH("/usr/share/fonts/truetype/uralic/gothub__.ttf","Gothic Uralic:style=Regular")

	// Check a couple of common paths for Gothic Uralic/bold as a last resort
	// Debian
	/*
	TRANSLATORS: If using the FTGL backend, this should be the path of a bold
	font that contains all the Unicode characters in use in	your translation.
	If the font is available in Debian it should be the Debian path.
	*/
	CHECK_FONT_PATH("/usr/share/fonts/truetype/uralic/gothub__.ttf","Gothic Uralic:style=Regular")
	/*
	TRANSLATORS: If using the FTGL backend, this should be the path of a
	font that contains all the Unicode characters in use in	your translation.
	If the font is available in Debian it should be the Debian path.
	*/
	CHECK_FONT_PATH("/usr/share/fonts/truetype/uralic/gothu___.ttf","Gothic Uralic:style=Regular")
	// Mandrake
	/*
	TRANSLATORS: If using the FTGL backend, this should be the path of a bold
	font that contains all the Unicode characters in use in	your translation.
	If the font is available in Mandrake it should be the Mandrake path.
	*/
	CHECK_FONT_PATH("/usr/share/fonts/TTF/uralic/GOTHUB__.TTF","Gothic Uralic:style=Bold")
	/*
	TRANSLATORS: If using the FTGL backend, this should be the path of a
	font that contains all the Unicode characters in use in	your translation.
	If the font is available in Mandrake it should be the Mandrake path.
	*/
	CHECK_FONT_PATH("/usr/share/fonts/TTF/uralic/GOTHU___.TTF","Gothic Uralic:style=Regular")

	// Check the non-translated versions of the above
	CHECK_FONT_PATH("/usr/share/fonts/truetype/uralic/gothub__.ttf","Gothic Uralic:style=Regular")
	CHECK_FONT_PATH("/usr/share/fonts/truetype/uralic/gothu___.ttf","Gothic Uralic:style=Regular")
	CHECK_FONT_PATH("/usr/share/fonts/TTF/uralic/GOTHUB__.TTF","Gothic Uralic:style=Regular")
	CHECK_FONT_PATH("/usr/share/fonts/TTF/uralic/GOTHU___.TTF","Gothic Uralic:style=Regular")

	CHECK_FONT_PATH("/usr/share/fonts/truetype/linux-libertine/LinLibertine_Re.ttf","Linux Libertine O:style=Regular")

	CHECK_FONT_PATH("/usr/share/fonts/truetype/freefont/FreeSerif.ttf","FreeSerif")
	CHECK_FONT_PATH("/usr/share/fonts/truetype/freefont/FreeSans.ttf","FreeSans")
	CHECK_FONT_PATH("/usr/share/fonts/truetype/freefont/FreeMono.ttf","FreeMono")

	//openSUSE
	CHECK_FONT_PATH("/usr/share/fonts/truetype/LinBiolinum_RB.otf","Bold")
	CHECK_FONT_PATH("/usr/share/fonts/truetype/FreeSerif.ttf","FreeSerif")
	CHECK_FONT_PATH("/usr/share/fonts/truetype/FreeSans.ttf","FreeSans")
	CHECK_FONT_PATH("/usr/share/fonts/truetype/FreeMono.ttf","FreeMono")

	// gentoo paths
	CHECK_FONT_PATH("/usr/share/fonts/freefont-ttf/FreeSerif.ttf","FreeSerif")
	CHECK_FONT_PATH("/usr/share/fonts/freefont-ttf/FreeSans.ttf","FreeSans")
	CHECK_FONT_PATH("/usr/share/fonts/freefont-ttf/FreeMono.ttf","FreeMono")

#ifdef _WIN32
	CHECK_FONT_PATH("c:\\windows\\fonts\\verdana.ttf",NULL)
	CHECK_FONT_PATH("c:\\windows\\fonts\\tahoma.ttf",NULL)
	CHECK_FONT_PATH("c:\\windows\\fonts\\arial.ttf",NULL)
	CHECK_FONT_PATH("\\windows\\fonts\\arial.ttf",NULL)
#endif

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Final selection of font file is [%s]\n",(font != NULL ? font : "null")); \

	return font;
}


}}//end namespace
