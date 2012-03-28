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

#ifndef _GLEST_GAME_AI_H_
#define _GLEST_GAME_AI_H_

#include <vector>
#include <list>

#include "world.h"
#include "commander.h"
#include "command.h"
#include "randomgen.h"
#include "leak_dumper.h"

using std::deque;
using std::vector;
using std::list;
using Shared::Util::RandomGen;

namespace Glest{ namespace Game{

class AiInterface;
class AiRule;

// =====================================================
// 	class Task  
//
///	An action that has to be performed by the IA
// =====================================================

enum TaskClass{
	tcProduce,
	tcBuild,
	tcUpgrade
};	

class Task{
protected:
	TaskClass taskClass;	

public:
	Task();
	virtual ~Task(){}
	TaskClass getClass() const	{return taskClass;}
	virtual string toString() const= 0;

	virtual void saveGame(XmlNode *rootNode) const = 0;
};

// ==================== ProduceTask ====================

class ProduceTask: public Task{
private:
	UnitClass unitClass;
	const UnitType *unitType;
	const ResourceType *resourceType;

	ProduceTask();
public:
	ProduceTask(UnitClass unitClass);
	ProduceTask(const UnitType *unitType);
	ProduceTask(const ResourceType *resourceType);
	
	UnitClass getUnitClass() const					{return unitClass;} 
	const UnitType *getUnitType() const				{return unitType;}
	const ResourceType *getResourceType() const		{return resourceType;}
	virtual string toString() const;

	virtual void saveGame(XmlNode *rootNode) const;
	static ProduceTask * loadGame(const XmlNode *rootNode, Faction *faction);
};

// ==================== BuildTask ====================

class BuildTask: public Task{
private:
	const UnitType *unitType;
	const ResourceType *resourceType;
	bool forcePos;
	Vec2i pos;

	BuildTask();

public:
	BuildTask(const UnitType *unitType);
	BuildTask(const ResourceType *resourceType);
	BuildTask(const UnitType *unitType, const Vec2i &pos);

	const UnitType *getUnitType() const			{return unitType;}
	const ResourceType *getResourceType() const	{return resourceType;}
	bool getForcePos() const					{return forcePos;}
	Vec2i getPos() const						{return pos;}
	virtual string toString() const;

	virtual void saveGame(XmlNode *rootNode) const;
	static BuildTask * loadGame(const XmlNode *rootNode, Faction *faction);
};

// ==================== UpgradeTask ====================

class UpgradeTask: public Task{
private:
	const UpgradeType *upgradeType;

	UpgradeTask();
public:
	UpgradeTask(const UpgradeType *upgradeType);
	const UpgradeType *getUpgradeType() const	{return upgradeType;}
	virtual string toString() const;

	virtual void saveGame(XmlNode *rootNode) const;
	static UpgradeTask * loadGame(const XmlNode *rootNode, Faction *faction);
};

// ===============================
// 	class AI 
//
///	Main AI class
// ===============================

class Ai {
private:
	static const int harvesterPercent= 30;
	static const int maxBuildRadius= 40; 
	static const int minMinWarriors= 7;
	static const int maxMinWarriors= 20;
	static const int minStaticResources= 10;
	static const int minConsumableResources= 20;
	static const int maxExpansions= 2;
	static const int villageRadius= 15;

public:
	enum ResourceUsage {
		ruHarvester,
		ruWarrior,
		ruBuilding,
		ruUpgrade
	};

private:
	//typedef vector<AiRule*> AiRules;
	typedef vector<AiRule *> AiRules;
	typedef list<const Task*> Tasks;
	typedef deque<Vec2i> Positions;

private:
    AiInterface *aiInterface;
	AiRules aiRules;
    int startLoc;
    bool randomMinWarriorsReached;
	Tasks tasks;
	Positions expansionPositions;
	RandomGen random;
	std::map<int,int> factionSwitchTeamRequestCount;

	bool getAdjacentUnits(std::map<float, std::map<int, const Unit *> > &signalAdjacentUnits, const Unit *unit);

public: 
	Ai() {
	    aiInterface = NULL;
	    startLoc = -1;
	    randomMinWarriorsReached = false;
	    minWarriors = 0;
	}
    ~Ai();

    int minWarriors;

	void init(AiInterface *aiInterface,int useStartLocation=-1);
    void update(); 

    //state requests
	AiInterface *getAiInterface() const		{return aiInterface;}
	RandomGen* getRandom()					{return &random;}
    int getCountOfType(const UnitType *ut);
	
	int getCountOfClass(UnitClass uc,UnitClass *additionalUnitClassToExcludeFromCount=NULL);
	float getRatioOfClass(UnitClass uc,UnitClass *additionalUnitClassToExcludeFromCount=NULL);

	const ResourceType *getNeededResource(int unitIndex);
	bool isStableBase();
	bool findPosForBuilding(const UnitType* building, const Vec2i &searchPos, Vec2i &pos);
	bool findAbleUnit(int *unitIndex, CommandClass ability, bool idleOnly);
	bool findAbleUnit(int *unitIndex, CommandClass ability, CommandClass currentCommand);
	vector<int> findUnitsDoingCommand(CommandClass currentCommand);
	vector<int> findUnitsHarvestingResourceType(const ResourceType *rt);

	bool beingAttacked(Vec2i &pos, Field &field, int radius);

	//tasks
	void addTask(const Task *task);
	void addPriorityTask(const Task *task);
	bool anyTask();
	const Task *getTask() const;
	void removeTask(const Task *task);
	void retryTask(const Task *task);

	//expansions
	void addExpansion(const Vec2i &pos);
	Vec2i getRandomHomePosition();

    //actions
    void sendScoutPatrol();
    void massiveAttack(const Vec2i &pos, Field field, bool ultraAttack= false);
    void returnBase(int unitIndex);
    void harvest(int unitIndex);
    bool haveBlockedUnits();
    void unblockUnits();

    bool outputAIBehaviourToConsole() const;

    void saveGame(XmlNode *rootNode) const;
    void loadGame(const XmlNode *rootNode, Faction *faction);
};

}}//end namespace

#endif
