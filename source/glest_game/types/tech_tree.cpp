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

#include "tech_tree.h"

#include <cassert>

#include "util.h"
#include "resource.h"
#include "faction_type.h"
#include "logger.h"
#include "xml_parser.h"
#include "platform_util.h"
#include "game_util.h"
#include "window.h"
#include "leak_dumper.h"

using namespace Shared::Util;
using namespace Shared::Xml;

namespace Glest{ namespace Game{

// =====================================================
// 	class TechTree
// =====================================================

TechTree::TechTree(const vector<string> pathList) {
	SkillType::resetNextAttackBoostId();

	name="";
	treePath="";
	this->pathList.assign(pathList.begin(), pathList.end());

    resourceTypes.clear();
    factionTypes.clear();
	armorTypes.clear();
	attackTypes.clear();
}

Checksum TechTree::loadTech(const string &techName,
		set<string> &factions, Checksum* checksum, std::map<string,vector<pair<string, string> > > &loadedFileList) {
	name = "";
    Checksum techtreeChecksum;
    string path=findPath(techName);
    if(path!="") {
    	//printf(">>> path=%s\n",path.c_str());
        load(path, factions, checksum, &techtreeChecksum, loadedFileList);
    }
    else {
    	printf(">>> techtree [%s] path not found.\n",techName.c_str());
    }
    return techtreeChecksum;
}

string TechTree::findPath(const string &techName) const {
	return findPath(techName,pathList);
}
string TechTree::findPath(const string &techName, const vector<string> &pathTechList) {
    for(unsigned int idx = 0; idx < pathTechList.size(); ++idx) {
    	string currentPath = (pathTechList)[idx];
    	endPathWithSlash(currentPath);

        string path = currentPath + techName;

        //printf(">>> test path=%s\n",path.c_str());
        if(isdir(path.c_str()) == true) {
            return path;
            //break;
        }
    }
    //return "no path found for tech: \""+techname+"\"";
    return "";
}


void TechTree::load(const string &dir, set<string> &factions, Checksum* checksum,
		Checksum *techtreeChecksum, std::map<string,vector<pair<string, string> > > &loadedFileList) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	string currentPath = dir;
	endPathWithSlash(currentPath);
	treePath = currentPath;
	name= lastDir(currentPath);

	char szBuf[8096]="";
	snprintf(szBuf,8096,Lang::getInstance().get("LogScreenGameLoadingTechtree","",true).c_str(),formatString(name).c_str());
	Logger::getInstance().add(szBuf, true);

	vector<string> filenames;
	//load resources
	string str= currentPath + "resources/*.";

    try {
        findAll(str, filenames);
		resourceTypes.resize(filenames.size());

        for(int i=0; i<filenames.size(); ++i){
            str= currentPath + "resources/" + filenames[i];
            resourceTypes[i].load(str, checksum, &checksumValue, loadedFileList,
            					treePath);
            Window::handleEvent();
			SDL_PumpEvents();
        }

        // Cleanup pixmap memory
        for(int i = 0; i < filenames.size(); ++i) {
        	resourceTypes[i].deletePixels();
        }
    }
    catch(const exception &e){
    	SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,e.what());
		throw megaglest_runtime_error("Error loading Resource Types in: [" + currentPath + "]\n" + e.what());
    }

    // give CPU time to update other things to avoid apperance of hanging
    sleep(0);
    Window::handleEvent();
	SDL_PumpEvents();

	//load tech tree xml info
	try {
		XmlTree	xmlTree;
    	string currentPath = dir;
    	endPathWithSlash(currentPath);
		string path = currentPath + lastDir(dir) + ".xml";

		checksum->addFile(path);
		checksumValue.addFile(path);

		std::map<string,string> mapExtraTagReplacementValues;
		mapExtraTagReplacementValues["$COMMONDATAPATH"] = currentPath + "/commondata/";
		xmlTree.load(path, Properties::getTagReplacementValues(&mapExtraTagReplacementValues));
		loadedFileList[path].push_back(make_pair(currentPath,currentPath));

		const XmlNode *techTreeNode= xmlTree.getRootNode();

		//attack types
		const XmlNode *attackTypesNode= techTreeNode->getChild("attack-types");
		attackTypes.resize(attackTypesNode->getChildCount());
		for(int i = 0; i < attackTypes.size(); ++i) {
			const XmlNode *attackTypeNode= attackTypesNode->getChild("attack-type", i);
			attackTypes[i].setName(attackTypeNode->getAttribute("name")->getRestrictedValue());
			attackTypes[i].setId(i);

			Window::handleEvent();
			SDL_PumpEvents();
		}

	    // give CPU time to update other things to avoid apperance of hanging
	    sleep(0);
		//SDL_PumpEvents();

		//armor types
		const XmlNode *armorTypesNode= techTreeNode->getChild("armor-types");
		armorTypes.resize(armorTypesNode->getChildCount());
		for(int i = 0; i < armorTypes.size(); ++i) {
			const XmlNode *armorTypeNode= armorTypesNode->getChild("armor-type", i);
			armorTypes[i].setName(armorTypeNode->getAttribute("name")->getRestrictedValue());
			armorTypes[i].setId(i);

			Window::handleEvent();
			SDL_PumpEvents();
		}

		//damage multipliers
		damageMultiplierTable.init(attackTypes.size(), armorTypes.size());
		const XmlNode *damageMultipliersNode= techTreeNode->getChild("damage-multipliers");
		for(int i = 0; i < damageMultipliersNode->getChildCount(); ++i) {
			const XmlNode *damageMultiplierNode= damageMultipliersNode->getChild("damage-multiplier", i);
			const AttackType *attackType= getAttackType(damageMultiplierNode->getAttribute("attack")->getRestrictedValue());
			const ArmorType *armorType= getArmorType(damageMultiplierNode->getAttribute("armor")->getRestrictedValue());
			float multiplier= damageMultiplierNode->getAttribute("value")->getFloatValue();
			damageMultiplierTable.setDamageMultiplier(attackType, armorType, multiplier);

			Window::handleEvent();
			SDL_PumpEvents();
		}
    }
    catch(const exception &e){
    	SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,e.what());
		throw megaglest_runtime_error("Error loading Tech Tree: "+ currentPath + "\n" + e.what());
    }

    // give CPU time to update other things to avoid apperance of hanging
    sleep(0);
	//SDL_PumpEvents();

	//load factions
    try{
		factionTypes.resize(factions.size());

		int i=0;
		for ( set<string>::iterator it = factions.begin(); it != factions.end(); ++it ) {
			string factionName = *it;

		    char szBuf[8096]="";
		    snprintf(szBuf,8096,"%s %s [%d / %d] - %s",Lang::getInstance().get("Loading").c_str(),
		    		Lang::getInstance().get("Faction").c_str(),
		    		i+1,
		    		(int)factions.size(),
		    		factionName.c_str());
		    Logger &logger= Logger::getInstance();
		    logger.setState(szBuf);
		    logger.setProgress((int)((((double)i) / (double)factions.size()) * 100.0));

			factionTypes[i++].load(factionName, this, checksum,&checksumValue,loadedFileList);

		    // give CPU time to update other things to avoid apperance of hanging
		    sleep(0);
		    Window::handleEvent();
			SDL_PumpEvents();
        }
    }
    catch(megaglest_runtime_error& ex) {
    	//printf("1111111b ex.wantStackTrace() = %d\n",ex.wantStackTrace());
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		//printf("222222b\n");
		throw megaglest_runtime_error("Error loading Faction Types: "+ currentPath + "\n" + ex.what(),!ex.wantStackTrace());
    }
	catch(const exception &e){
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,e.what());
		throw megaglest_runtime_error("Error loading Faction Types: "+ currentPath + "\n" + e.what());
    }

    if(techtreeChecksum != NULL) {
        *techtreeChecksum = checksumValue;
    }

    Lang &lang = Lang::getInstance();
    lang.loadTechTreeStrings(name);

    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

TechTree::~TechTree() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	Logger::getInstance().add(Lang::getInstance().get("LogScreenGameUnLoadingTechtree","",true), true);
	resourceTypes.clear();
	factionTypes.clear();
	armorTypes.clear();
	attackTypes.clear();
}

std::vector<std::string> TechTree::validateFactionTypes() {
	std::vector<std::string> results;
	for (int i = 0; i < factionTypes.size(); ++i) {
		std::vector<std::string> factionResults = factionTypes[i].validateFactionType();
		if(factionResults.empty() == false) {
			results.insert(results.end(), factionResults.begin(), factionResults.end());
		}

		factionResults = factionTypes[i].validateFactionTypeUpgradeTypes();
		if(factionResults.empty() == false) {
			results.insert(results.end(), factionResults.begin(), factionResults.end());
		}
	}

	return results;
}

std::vector<std::string> TechTree::validateResourceTypes() {
	std::vector<std::string> results;
	ResourceTypes resourceTypesNotUsed = resourceTypes;
	for (unsigned int i = 0; i < resourceTypesNotUsed.size(); ++i) {
		ResourceType &rt = resourceTypesNotUsed[i];
		rt.setCleanupMemory(false);
	}
	for (unsigned int i = 0; i < factionTypes.size(); ++i) {
		//printf("Validating [%d / %d] faction [%s]\n",i,(int)factionTypes.size(),factionTypes[i].getName().c_str());

		std::vector<std::string> factionResults = factionTypes[i].validateFactionTypeResourceTypes(resourceTypes);
		if(factionResults.empty() == false) {
			results.insert(results.end(), factionResults.begin(), factionResults.end());
		}

		// Check if the faction uses the resources in this techtree
		// Now lets find a matching faction resource type for the unit
		for(int j = resourceTypesNotUsed.size() -1; j >= 0; --j) {
			const ResourceType &rt = resourceTypesNotUsed[j];
			//printf("Validating [%d / %d] resourcetype [%s]\n",j,(int)resourceTypesNotUsed.size(),rt.getName().c_str());

			if(factionTypes[i].factionUsesResourceType(&rt) == true) {
				//printf("FOUND FACTION CONSUMER FOR RESOURCE - [%d / %d] faction [%s] resource [%d / %d] resourcetype [%s]\n",i,(int)factionTypes.size(),factionTypes[i].getName().c_str(),j,(int)resourceTypesNotUsed.size(),rt.getName().c_str());
				resourceTypesNotUsed.erase(resourceTypesNotUsed.begin() + j);
			}
		}
	}

	if(resourceTypesNotUsed.empty() == false) {
		//printf("FOUND unused resource Types [%d]\n",(int)resourceTypesNotUsed.size());

		for (unsigned int i = 0; i < resourceTypesNotUsed.size(); ++i) {
			const ResourceType &rt = resourceTypesNotUsed[i];
			char szBuf[8096]="";
			snprintf(szBuf,8096,"The Resource type [%s] is not used by any units in this techtree!",rt.getName().c_str());
			results.push_back(szBuf);
		}
	}
    return results;
}

// ==================== get ====================

FactionType *TechTree::getTypeByName(const string &name) {
    for(int i=0; i < factionTypes.size(); ++i) {
          if(factionTypes[i].getName() == name) {
               return &factionTypes[i];
          }
    }
    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
    throw megaglest_runtime_error("Faction not found: "+name);
}

const FactionType *TechTree::getType(const string &name) const {
    for(int i=0; i < factionTypes.size(); ++i) {
          if(factionTypes[i].getName() == name) {
               return &factionTypes[i];
          }
    }
    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
    throw megaglest_runtime_error("Faction not found: "+name);
}

const ResourceType *TechTree::getTechResourceType(int i) const{
     for(int j=0; j < getResourceTypeCount(); ++j){
          const ResourceType *rt= getResourceType(j);
          assert(rt != NULL);
          if(rt == NULL) {
        	  throw megaglest_runtime_error("rt == NULL");
          }
          if(rt->getResourceNumber() == i && rt->getClass() == rcTech)
               return getResourceType(j);
     }

     return getFirstTechResourceType();
}

const ResourceType *TechTree::getFirstTechResourceType() const{
     for(int i=0; i<getResourceTypeCount(); ++i){
          const ResourceType *rt= getResourceType(i);
          assert(rt != NULL);
          if(rt->getResourceNumber()==1 && rt->getClass()==rcTech)
               return getResourceType(i);
     }

     char szBuf[8096]="";
     snprintf(szBuf,8096,"The referenced tech tree [%s] is either missing or has no resources defined but at least one resource is required.",this->name.c_str());
	 //throw megaglest_runtime_error("This tech tree has no resources defined, at least one is required");
     throw megaglest_runtime_error(szBuf);
}

const ResourceType *TechTree::getResourceType(const string &name) const{

	for(int i=0; i<resourceTypes.size(); ++i){
		if(resourceTypes[i].getName()==name){
			return &resourceTypes[i];
		}
	}

	throw megaglest_runtime_error("Resource Type not found: "+name);
}

const ArmorType *TechTree::getArmorType(const string &name) const{
	for(int i=0; i<armorTypes.size(); ++i){
		if(armorTypes[i].getName()==name){
			return &armorTypes[i];
		}
	}

	throw megaglest_runtime_error("Armor Type not found: "+name);
}

const AttackType *TechTree::getAttackType(const string &name) const{
	for(int i=0; i<attackTypes.size(); ++i){
		if(attackTypes[i].getName()==name){
			return &attackTypes[i];
		}
	}

	throw megaglest_runtime_error("Attack Type not found: "+name);
}

float TechTree::getDamageMultiplier(const AttackType *att, const ArmorType *art) const{
	return damageMultiplierTable.getDamageMultiplier(att, art);
}

void TechTree::saveGame(XmlNode *rootNode) {
	std::map<string,string> mapTagReplacements;
	XmlNode *techTreeNode = rootNode->addChild("TechTree");

//	string name;
	techTreeNode->addAttribute("name",name, mapTagReplacements);
//    //string desc;
//	string treePath;
	//techTreeNode->addAttribute("treePath",treePath, mapTagReplacements);
//	vector<string> pathList;
//	for(unsigned int i = 0; i < pathList.size(); ++i) {
//		XmlNode *pathListNode = techTreeNode->addChild("pathList");
//		pathListNode->addAttribute("value",pathList[i], mapTagReplacements);
//	}
//    ResourceTypes resourceTypes;
//	for(unsigned int i = 0; i < resourceTypes.size(); ++i) {
//		ResourceType &rt = resourceTypes[i];
//		rt.saveGame(techTreeNode);
//	}
//    FactionTypes factionTypes;
//	for(unsigned int i = 0; i < factionTypes.size(); ++i) {
//		FactionType &ft = factionTypes[i];
//		ft.saveGame(techTreeNode);
//	}

//	ArmorTypes armorTypes;
//	for(unsigned int i = 0; i < armorTypes.size(); ++i) {
//		ArmorType &at = armorTypes[i];
//		at.saveGame(techTreeNode);
//	}

//	AttackTypes attackTypes;
//	for(unsigned int i = 0; i < attackTypes.size(); ++i) {
//		AttackType &at = attackTypes[i];
//		at.saveGame(techTreeNode);
//	}

//	DamageMultiplierTable damageMultiplierTable;
//	damageMultiplierTable.saveGame(techTreeNode);

//	Checksum checksumValue;
	techTreeNode->addAttribute("checksumValue",intToStr(checksumValue.getSum()), mapTagReplacements);
}

}}//end namespace
