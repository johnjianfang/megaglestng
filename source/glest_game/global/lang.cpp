// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "lang.h"

#include <stdexcept>

#include "logger.h"
#include "util.h"
#include "platform_util.h"
#include "game_constants.h"
#include "game_util.h"
#include "platform_common.h"
#include "conversion.h"
#include "gl_wrap.h"
#include "core_data.h"
#include "renderer.h"
#include <algorithm>
#include "config.h"
#include "leak_dumper.h"

using namespace std;
using namespace Shared::Util;
using namespace Shared::Platform;

namespace Glest{ namespace Game{

const char *DEFAULT_LANGUAGE = "english";

// =====================================================
// 	class Lang
// =====================================================

Lang::Lang() {
	language 			= "";
	is_utf8_language 	= false;
}

Lang &Lang::getInstance() {
	static Lang lang;
	return lang;
}

string Lang::getDefaultLanguage() const {
	return DEFAULT_LANGUAGE;
}

void Lang::loadStrings(string uselanguage, bool loadFonts,
		bool fallbackToDefault) {
	if(uselanguage.length() == 2 || (uselanguage.length() == 5 && uselanguage[2] == '-')) {
		uselanguage = getLanguageFile(uselanguage);
	}
	bool languageChanged = (uselanguage != this->language);
	this->language= uselanguage;
	loadStrings(uselanguage, strings, true, fallbackToDefault);

	if(languageChanged == true) {
		Font::resetToDefaults();
		Lang &lang = Lang::getInstance();
		if(	lang.hasString("FONT_BASE_SIZE")) {
			Font::baseSize    = strToInt(lang.get("FONT_BASE_SIZE"));
		}

		if(	lang.hasString("FONT_SCALE_SIZE")) {
			Font::scaleFontValue = strToFloat(lang.get("FONT_SCALE_SIZE"));
		}
		if(	lang.hasString("FONT_SCALE_CENTERH_FACTOR")) {
			Font::scaleFontValueCenterHFactor = strToFloat(lang.get("FONT_SCALE_CENTERH_FACTOR"));
		}

		if(	lang.hasString("FONT_CHARCOUNT")) {
			// 256 for English
			// 30000 for Chinese
			Font::charCount    = strToInt(lang.get("FONT_CHARCOUNT"));
		}
		if(	lang.hasString("FONT_TYPENAME")) {
			Font::fontTypeName = lang.get("FONT_TYPENAME");
		}
		if(	lang.hasString("FONT_CHARSET")) {
			// Example values:
			// DEFAULT_CHARSET (English) = 1
			// GB2312_CHARSET (Chinese)  = 134
			Shared::Platform::charSet = strToInt(lang.get("FONT_CHARSET"));
		}
		if(	lang.hasString("FONT_MULTIBYTE")) {
			Font::fontIsMultibyte 	= strToBool(lang.get("FONT_MULTIBYTE"));
		}

		if(	lang.hasString("FONT_RIGHTTOLEFT")) {
			Font::fontIsRightToLeft	= strToBool(lang.get("FONT_RIGHTTOLEFT"));
		}

		if(	lang.hasString("MEGAGLEST_FONT")) {
			//setenv("MEGAGLEST_FONT","/usr/share/fonts/truetype/ttf-japanese-gothic.ttf",0); // Japanese
	#if defined(WIN32)
			string newEnvValue = "MEGAGLEST_FONT=" + lang.get("MEGAGLEST_FONT");
			_putenv(newEnvValue.c_str());
	#else
			setenv("MEGAGLEST_FONT",lang.get("MEGAGLEST_FONT").c_str(),0);
	#endif
		}

		if(	lang.hasString("MEGAGLEST_FONT_FAMILY")) {
	#if defined(WIN32)
			string newEnvValue = "MEGAGLEST_FONT_FAMILY=" + lang.get("MEGAGLEST_FONT_FAMILY");
			_putenv(newEnvValue.c_str());
	#else
			setenv("MEGAGLEST_FONT_FAMILY",lang.get("MEGAGLEST_FONT_FAMILY").c_str(),0);
	#endif
		}

	//        if(	lang.hasString("FONT_YOFFSET_FACTOR")) {
	//        	FontMetrics::DEFAULT_Y_OFFSET_FACTOR = strToFloat(lang.get("FONT_YOFFSET_FACTOR"));
	//		}

	#if defined(WIN32)
		// Win32 overrides for fonts (just in case they must be different)

		if(	lang.hasString("FONT_BASE_SIZE_WINDOWS")) {
			// 256 for English
			// 30000 for Chinese
			Font::baseSize    = strToInt(lang.get("FONT_BASE_SIZE_WINDOWS"));
		}

		if(	lang.hasString("FONT_SCALE_SIZE_WINDOWS")) {
			Font::scaleFontValue = strToFloat(lang.get("FONT_SCALE_SIZE_WINDOWS"));
		}
		if(	lang.hasString("FONT_SCALE_CENTERH_FACTOR_WINDOWS")) {
			Font::scaleFontValueCenterHFactor = strToFloat(lang.get("FONT_SCALE_CENTERH_FACTOR_WINDOWS"));
		}

		if(	lang.hasString("FONT_HEIGHT_TEXT_WINDOWS")) {
			Font::langHeightText = lang.get("FONT_HEIGHT_TEXT_WINDOWS",Font::langHeightText.c_str());
		}

		if(	lang.hasString("FONT_CHARCOUNT_WINDOWS")) {
			// 256 for English
			// 30000 for Chinese
			Font::charCount    = strToInt(lang.get("FONT_CHARCOUNT_WINDOWS"));
		}
		if(	lang.hasString("FONT_TYPENAME_WINDOWS")) {
			Font::fontTypeName = lang.get("FONT_TYPENAME_WINDOWS");
		}
		if(	lang.hasString("FONT_CHARSET_WINDOWS")) {
			// Example values:
			// DEFAULT_CHARSET (English) = 1
			// GB2312_CHARSET (Chinese)  = 134
			Shared::Platform::charSet = strToInt(lang.get("FONT_CHARSET_WINDOWS"));
		}
		if(	lang.hasString("FONT_MULTIBYTE_WINDOWS")) {
			Font::fontIsMultibyte 	= strToBool(lang.get("FONT_MULTIBYTE_WINDOWS"));
		}
		if(	lang.hasString("FONT_RIGHTTOLEFT_WINDOWS")) {
			Font::fontIsRightToLeft	= strToBool(lang.get("FONT_RIGHTTOLEFT_WINDOWS"));
		}

		if(	lang.hasString("MEGAGLEST_FONT_WINDOWS")) {
			//setenv("MEGAGLEST_FONT","/usr/share/fonts/truetype/ttf-japanese-gothic.ttf",0); // Japanese
			string newEnvValue = "MEGAGLEST_FONT=" + lang.get("MEGAGLEST_FONT_WINDOWS");
			_putenv(newEnvValue.c_str());
		}

	//        if(	lang.hasString("FONT_YOFFSET_FACTOR_WINDOWS")) {
	//        	FontMetrics::DEFAULT_Y_OFFSET_FACTOR = strToFloat(lang.get("FONT_YOFFSET_FACTOR_WINDOWS"));
	//		}

		// end win32
	#endif

		if(loadFonts == true) {
			CoreData &coreData= CoreData::getInstance();
			coreData.loadFonts();
		}
    }
}

void Lang::loadStrings(string uselanguage, Properties &properties, bool fileMustExist,
		bool fallbackToDefault) {
	properties.clear();
	string data_path = getGameReadWritePath(GameConstants::path_data_CacheLookupKey);
	//string languageFile = data_path + "data/lang/" + uselanguage + ".lng";
	string languageFile = getGameCustomCoreDataPath(data_path, "data/lang/" + uselanguage + ".lng");
	if(fileMustExist == false && fileExists(languageFile) == false) {
		return;
	}
	else if(fileExists(languageFile) == false && fallbackToDefault == true) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] path = [%s]\n",__FILE__,__FUNCTION__,__LINE__,languageFile.c_str());
		//throw megaglest_runtime_error("File NOT FOUND, can't open file: [" + languageFile + "]");
		printf("Language file NOT FOUND, can't open file: [%s] switching to default language: %s\n",languageFile.c_str(),DEFAULT_LANGUAGE);

		languageFile = getGameCustomCoreDataPath(data_path, "data/lang/" + string(DEFAULT_LANGUAGE) + ".lng");
	}
	is_utf8_language = valid_utf8_file(languageFile.c_str());
	properties.load(languageFile);
}

bool Lang::isUTF8Language() const {
	return is_utf8_language;
}

void Lang::loadScenarioStrings(string scenarioDir, string scenarioName){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] scenarioDir = [%s] scenarioName = [%s]\n",__FILE__,__FUNCTION__,__LINE__,scenarioDir.c_str(),scenarioName.c_str());

	// First try to find scenario lng file in userdata
	Config &config = Config::getInstance();
    vector<string> scenarioPaths = config.getPathListForType(ptScenarios);
    if(scenarioPaths.size() > 1) {
        string &scenarioPath = scenarioPaths[1];
		endPathWithSlash(scenarioPath);

		string currentPath = scenarioPath;
		endPathWithSlash(currentPath);
		string scenarioFolder = currentPath + scenarioName + "/";
		string path = scenarioFolder + scenarioName + "_" + language + ".lng";

		//try to load the current language first
		if(fileExists(path)) {
			scenarioDir = scenarioPath;
		}
    }

	string currentPath = scenarioDir;
	endPathWithSlash(currentPath);
	string scenarioFolder = currentPath + scenarioName + "/";
	string path = scenarioFolder + scenarioName + "_" + language + ".lng";
	if(EndsWith(scenarioDir, ".xml") == true) {
		scenarioFolder = extractDirectoryPathFromFile(scenarioDir);
		path = scenarioFolder + scenarioName + "_" + language + ".lng";

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] path = [%s]\n",__FILE__,__FUNCTION__,__LINE__,path.c_str());
	}

	scenarioStrings.clear();

	//try to load the current language first
	if(fileExists(path)) {
		scenarioStrings.load(path);
	}
	else{
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] path not found [%s]\n",__FILE__,__FUNCTION__,__LINE__,path.c_str());

		//try english otherwise
		path = scenarioFolder + scenarioName + "_english.lng";
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] path = [%s]\n",__FILE__,__FUNCTION__,__LINE__,path.c_str());

		if(fileExists(path)){
			scenarioStrings.load(path);
		}
	}
}

void Lang::loadTechTreeStrings(string techTree) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] techTree = [%s]\n",__FILE__,__FUNCTION__,__LINE__,techTree.c_str());

	string currentPath = "";
	Config &config = Config::getInstance();
    vector<string> techPaths = config.getPathListForType(ptTechs);
    for(int idx = 0; idx < techPaths.size(); idx++) {
        string &techPath = techPaths[idx];
		endPathWithSlash(techPath);

        //printf("techPath [%s]\n",techPath.c_str());

		if(folderExists(techPath + techTree) == true) {
			currentPath = techPath;
			endPathWithSlash(currentPath);
			break;
		}
    }

	string techTreeFolder = currentPath + techTree + "/";
	string path = techTreeFolder + "lang/" + techTree + "_" + language + ".lng";
	string pathDefault = techTreeFolder + "lang/" + techTree + "_default.lng";
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] path = [%s] pathDefault = [%s]\n",__FILE__,__FUNCTION__,__LINE__,path.c_str(),pathDefault.c_str());

	techTreeStrings.clear();
	techTreeStringsDefault.clear();

	//try to load the current language first
	if(fileExists(path)) {
		techTreeStrings.load(path);
	}
	else {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] path not found [%s]\n",__FILE__,__FUNCTION__,__LINE__,path.c_str());

		//try english otherwise
		path = techTreeFolder + "lang/" + techTree + "_english.lng";
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] path = [%s]\n",__FILE__,__FUNCTION__,__LINE__,path.c_str());

		if(fileExists(path)) {
			techTreeStrings.load(path);
		}
	}

	if(fileExists(pathDefault)) {
		techTreeStringsDefault.load(pathDefault);
	}
}

void Lang::loadTilesetStrings(string tileset) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] tileset = [%s]\n",__FILE__,__FUNCTION__,__LINE__,tileset.c_str());

	string currentPath = "";
	Config &config = Config::getInstance();
    vector<string> tilesetPaths = config.getPathListForType(ptTilesets);
    for(int idx = 0; idx < tilesetPaths.size(); idx++) {
        string &tilesetPath = tilesetPaths[idx];
		endPathWithSlash(tilesetPath);

        //printf("tilesetPath [%s]\n",tilesetPath.c_str());

		if(folderExists(tilesetPath + tileset) == true) {
			currentPath = tilesetPath;
			endPathWithSlash(currentPath);
			break;
		}
    }

	string tilesetFolder = currentPath + tileset + "/";
	string path = tilesetFolder + "lang/" + tileset + "_" + language + ".lng";
	string pathDefault = tilesetFolder + "lang/" + tileset + "_default.lng";
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] path = [%s] pathDefault = [%s]\n",__FILE__,__FUNCTION__,__LINE__,path.c_str(),pathDefault.c_str());

	tilesetStrings.clear();
	tilesetStringsDefault.clear();

	//try to load the current language first
	if(fileExists(path)) {
		tilesetStrings.load(path);
	}
	else {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] path not found [%s]\n",__FILE__,__FUNCTION__,__LINE__,path.c_str());

		//try english otherwise
		path = tilesetFolder + "lang/" + tileset + "_english.lng";
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] path = [%s]\n",__FILE__,__FUNCTION__,__LINE__,path.c_str());

		if(fileExists(path)) {
			tilesetStrings.load(path);
		}
	}

	if(fileExists(pathDefault)) {
		tilesetStringsDefault.load(pathDefault);
	}
}

bool Lang::hasString(const string &s, string uselanguage, bool fallbackToDefault) {
	bool result = false;
	try {
		if(uselanguage != "") {
			//printf("#a fallbackToDefault = %d [%s] uselanguage [%s] DEFAULT_LANGUAGE  [%s] this->language [%s]\n",fallbackToDefault,s.c_str(),uselanguage.c_str(),DEFAULT_LANGUAGE,this->language.c_str());
			if(otherLanguageStrings.find(uselanguage) == otherLanguageStrings.end()) {
				loadStrings(uselanguage, otherLanguageStrings[uselanguage], false);
			}
			//string result2 = otherLanguageStrings[uselanguage].getString(s);
			otherLanguageStrings[uselanguage].getString(s);
			//printf("#b result2 [%s]\n",result2.c_str());

			result = true;
		}
		else {
			//string result2 = strings.getString(s);
			strings.getString(s);
			result = true;
		}
	}
	catch(exception &ex) {
		if(strings.getpath() != "") {
			if(SystemFlags::VERBOSE_MODE_ENABLED) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s] for uselanguage [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what(),uselanguage.c_str());
		}

		//printf("#1 fallbackToDefault = %d [%s] uselanguage [%s] DEFAULT_LANGUAGE  [%s] this->language [%s]\n",fallbackToDefault,s.c_str(),uselanguage.c_str(),DEFAULT_LANGUAGE,this->language.c_str());
		if(fallbackToDefault == true && uselanguage != DEFAULT_LANGUAGE && this->language != DEFAULT_LANGUAGE) {
			result = hasString(s, DEFAULT_LANGUAGE, false);
		}
		else {

		}
	}
	return result;
}

bool Lang::isLanguageLocal(string compareLanguage) const {
	return (compareLanguage == language);
}

string Lang::parseResult(const string &key, const string &value) {
	if(value != "$USE_DEFAULT_LANGUAGE_VALUE") {
		return value;
	}
	string result = Lang::get(key, DEFAULT_LANGUAGE);
	return result;
}
string Lang::get(const string &s, string uselanguage, bool fallbackToDefault) {
	try {
		string result = "";

		if(uselanguage != "") {
			if(otherLanguageStrings.find(uselanguage) == otherLanguageStrings.end()) {
				loadStrings(uselanguage, otherLanguageStrings[uselanguage], false);
			}
			result = otherLanguageStrings[uselanguage].getString(s);
			replaceAll(result, "\\n", "\n");
		}
		else {
			result = strings.getString(s);
			replaceAll(result, "\\n", "\n");
		}

		return parseResult(s, result);;
	}
	catch(exception &ex) {
		if(strings.getpath() != "") {
			if(fallbackToDefault == false || SystemFlags::VERBOSE_MODE_ENABLED) {
				if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false) {
					if(SystemFlags::VERBOSE_MODE_ENABLED) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s] uselanguage [%s] text [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what(),uselanguage.c_str(),s.c_str());
				}
			}
		}

		//printf("#2 fallbackToDefault = %d [%s] uselanguage [%s] DEFAULT_LANGUAGE  [%s] this->language [%s]\n",fallbackToDefault,s.c_str(),uselanguage.c_str(),DEFAULT_LANGUAGE,this->language.c_str());

		//if(fallbackToDefault == true && uselanguage != DEFAULT_LANGUAGE && this->language != DEFAULT_LANGUAGE) {
		if( uselanguage != DEFAULT_LANGUAGE && this->language != DEFAULT_LANGUAGE) {
			return get(s, DEFAULT_LANGUAGE, false);
		}

		return "???" + s + "???";
	}
	return "???" + s + "???";
}

string Lang::getScenarioString(const string &s) {
	try{
		string result = scenarioStrings.getString(s);
		replaceAll(result, "\\n", "\n");
		return result;
	}
	catch(exception &ex) {
		if(scenarioStrings.getpath() != "") {
			SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		}
		return "???" + s + "???";
	}
}

bool Lang::hasScenarioString(const string &s) {
	bool result = false;
	try {
		scenarioStrings.getString(s);
		result = true;
	}
	catch(exception &ex) {
		if(scenarioStrings.getpath() != "") {
			if(SystemFlags::VERBOSE_MODE_ENABLED) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		}
	}
	return result;
}

string Lang::getTechTreeString(const string &s,const char *defaultValue) {
	try{
		string result = "";

		if(techTreeStrings.hasString(s) == true || defaultValue == NULL) {
			if(techTreeStrings.hasString(s) == false && techTreeStringsDefault.hasString(s) == true) {
				result = techTreeStringsDefault.getString(s);
			}
			else {
				result = techTreeStrings.getString(s);
			}
		}
		else if(techTreeStringsDefault.hasString(s) == true) {
			result = techTreeStringsDefault.getString(s);
		}
		else if(defaultValue != NULL) {
			result = defaultValue;
		}
		replaceAll(result, "\\n", "\n");
		return result;
	}
	catch(exception &ex) {
		if(techTreeStrings.getpath() != "") {
			SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		}
		return "???" + s + "???";
	}
	return "???" + s + "???";
}

string Lang::getTilesetString(const string &s,const char *defaultValue) {
	try{
		string result = "";

		if(tilesetStrings.hasString(s) == true || defaultValue == NULL) {
			if(tilesetStrings.hasString(s) == false && tilesetStringsDefault.hasString(s) == true) {
				result = tilesetStringsDefault.getString(s);
			}
			else {
				result = tilesetStrings.getString(s);
			}
		}
		else if(tilesetStringsDefault.hasString(s) == true) {
			result = tilesetStringsDefault.getString(s);
		}
		else if(defaultValue != NULL) {
			result = defaultValue;
		}
		replaceAll(result, "\\n", "\n");
		return result;
	}
	catch(exception &ex) {
		if(tilesetStrings.getpath() != "") {
			SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		}
		return "???" + s + "???";
	}
	return "???" + s + "???";
}

bool Lang::fileMatchesISO630Code(string uselanguage, string testLanguageFile) {
	bool result = false;
	Properties stringsTest;
	stringsTest.load(testLanguageFile);

	try {
		string iso639 = stringsTest.getString("ISO639-1");
		if(iso639 == uselanguage) {
			result = true;
		}
	}
	//catch(const exception &ex) {
	catch(...) {
	}

	return result;
}

string Lang::getLanguageFile(string uselanguage) {
	bool foundMatch = false;
	string result = uselanguage;

    string data_path = getGameReadWritePath(GameConstants::path_data_CacheLookupKey);

    vector<string> langResults;
    string userDataPath = getGameCustomCoreDataPath(data_path, "");
	findAll(userDataPath + "data/lang/*.lng", langResults, true, false);
	for(unsigned int i = 0; i < langResults.size() && foundMatch == false ; ++i) {
		string testLanguageFile = userDataPath + "data/lang/" + langResults[i] + ".lng";
		foundMatch = fileMatchesISO630Code(uselanguage, testLanguageFile);
		if(foundMatch == true) {
			result = langResults[i];
		}
	}
	if(foundMatch == false) {
		langResults.clear();
		findAll(data_path + "data/lang/*.lng", langResults, true);
		for(unsigned int i = 0; i < langResults.size() && foundMatch == false ; ++i) {
			string testLanguageFile = data_path + "data/lang/" + langResults[i] + ".lng";
			foundMatch = fileMatchesISO630Code(uselanguage, testLanguageFile);
			if(foundMatch == true) {
				result = langResults[i];
			}
		}
	}
	return result;
}

string Lang::getNativeLanguageName(string uselanguage, string testLanguageFile) {
	string result = uselanguage;

	static map<string,string> cachedNativeLanguageNames;
	if(cachedNativeLanguageNames.find(testLanguageFile) != cachedNativeLanguageNames.end()) {
		result = cachedNativeLanguageNames[testLanguageFile];
	}
	else {
		Properties stringsTest;
		stringsTest.load(testLanguageFile);

		try {
			result = stringsTest.getString("NativeLanguageName");
			cachedNativeLanguageNames[testLanguageFile] = result;

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Caching native language name for [%s] = [%s]\n",testLanguageFile.c_str(),result.c_str());
		}
		catch(const exception &ex) {
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("ERROR Caching native language name for [%s] msg: [%s]\n",testLanguageFile.c_str(),ex.what());
		}
		catch(...) {
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("ERROR Caching native language name for [%s] msg: [UNKNOWN]\n",testLanguageFile.c_str());
		}
	}

	return result;
}

pair<string,string> Lang::getNavtiveNameFromLanguageName(string langName) {
	pair<string,string> result;

	//printf("looking for language [%s]\n",langName.c_str());

	map<string,string> nativeList = Lang::getDiscoveredLanguageList(true);
	map<string,string>::iterator iterMap = nativeList.find(langName);
	if(iterMap != nativeList.end()) {
		result = make_pair(iterMap->second,iterMap->first);
	}
	return result;
}

map<string,string> Lang::getDiscoveredLanguageList(bool searchKeyIsLangName) {
	map<string,string> result;

	string data_path = getGameReadWritePath(GameConstants::path_data_CacheLookupKey);
    string userDataPath = getGameCustomCoreDataPath(data_path, "");

    vector<string> langResults;
	findAll(userDataPath + "data/lang/*.lng", langResults, true, false);
	for(unsigned int i = 0; i < langResults.size(); ++i) {
		string testLanguage = langResults[i];
		string testLanguageFile = userDataPath + "data/lang/" + testLanguage + ".lng";
		string nativeName = getNativeLanguageName(testLanguage, testLanguageFile);
		if(searchKeyIsLangName == false) {
			result[nativeName] = testLanguage;
		}
		else {
			result[testLanguage] = nativeName;
		}
	}

	vector<string> langResults2;
	findAll(data_path + "data/lang/*.lng", langResults2, true);
	if(langResults2.empty() && langResults.empty()) {
        throw megaglest_runtime_error("There are no lang files");
	}
	for(unsigned int i = 0; i < langResults2.size(); ++i) {
		string testLanguage = langResults2[i];
		if(std::find(langResults.begin(),langResults.end(),testLanguage) == langResults.end()) {
			langResults.push_back(testLanguage);

			string testLanguageFile = data_path + "data/lang/" + testLanguage + ".lng";
			string nativeName = getNativeLanguageName(testLanguage, testLanguageFile);
			if(searchKeyIsLangName == false) {
				result[nativeName] = testLanguage;
			}
			else {
				result[testLanguage] = nativeName;
			}
		}
	}

	return result;
}

}}//end namespace
