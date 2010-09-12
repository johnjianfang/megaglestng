// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_MENUSTATECUSTOMGAME_H_
#define _GLEST_GAME_MENUSTATECUSTOMGAME_H_

#include "main_menu.h"
#include "chat_manager.h"
#include "simple_threads.h"
#include "game.h"
#include "leak_dumper.h"

namespace Glest{ namespace Game{

struct MapFileHeaderPreview {
	int32 version;
	int32 maxFactions;
	int32 width;
	int32 height;
	int32 altFactor;
	int32 waterLevel;
	int8 title[128];
	int8 author[128];
	int8 description[256];
};

// ===============================================
//	class Map
// ===============================================

class MapPreview {
public:
	static const int maxHeight = 20;
	static const int minHeight = 0;

private:
	struct Cell {
		int surface;
		int object;
		int resource;
		float height;
	};

	struct StartLocation {
		int x;
		int y;
	};

	RandomGen random;
	string title;
	string author;
	string desc;
	string recScn;
	int type;
	int h;
	int w;
	int altFactor;
	int waterLevel;
	Cell **cells;
	int maxFactions;
	StartLocation *startLocations;
	int refAlt;
	bool fileLoaded;

public:
	MapPreview();
	~MapPreview();
	float getHeight(int x, int y) const;
	int getSurface(int x, int y) const;
	int getObject(int x, int y) const;
	int getResource(int x, int y) const;
	int getStartLocationX(int index) const;
	int getStartLocationY(int index) const;
	int getHeightFactor() const;
	int getWaterLevel() const;
	bool inside(int x, int y);

	int getH() const			{return h;}
	int getW() const			{return w;}
	int getMaxFactions() const	{return maxFactions;}
	string getTitle() const		{return title;}
	string getDesc() const		{return desc;}
	string getAuthor() const	{return author;}

	void reset(int w, int h, float alt, int surf);
	void resetFactions(int maxFactions);

	void loadFromFile(const string &path);
	bool hasFileLoaded() const { return fileLoaded; }
};



// ===============================
// 	class MenuStateCustomGame
// ===============================

class MenuStateCustomGame : public MenuState, public SimpleTaskCallbackInterface {
private:
	GraphicButton buttonReturn;
	GraphicButton buttonPlayNow;
	GraphicButton buttonRestoreLastSettings;
	GraphicLabel labelControl;
	GraphicLabel labelFaction;
	GraphicLabel labelTeam;
	GraphicLabel labelMap;
	GraphicLabel labelFogOfWar;
	GraphicLabel labelTechTree;
	GraphicLabel labelTileset;
	GraphicLabel labelMapInfo;
	GraphicLabel labelEnableObserverMode;
	GraphicLabel labelEnableServerControlledAI;
	GraphicLabel labelLocalIP;
	

	GraphicListBox listBoxMap;
	GraphicListBox listBoxFogOfWar;
	GraphicListBox listBoxTechTree;
	GraphicListBox listBoxTileset;
	GraphicListBox listBoxEnableObserverMode;
	GraphicListBox listBoxEnableServerControlledAI;

	vector<string> mapFiles;
	vector<string> playerSortedMaps[GameConstants::maxPlayers+1];
	vector<string> formattedPlayerSortedMaps[GameConstants::maxPlayers+1];
	vector<string> techTreeFiles;
	vector<string> tilesetFiles;
	vector<string> factionFiles;
	GraphicLabel labelPlayers[GameConstants::maxPlayers];
	GraphicLabel labelPlayerNames[GameConstants::maxPlayers];
	GraphicListBox listBoxControls[GameConstants::maxPlayers];
	GraphicListBox listBoxFactions[GameConstants::maxPlayers];
	GraphicListBox listBoxTeams[GameConstants::maxPlayers];
	GraphicLabel labelNetStatus[GameConstants::maxPlayers];
	MapInfo mapInfo;
	
	GraphicLabel labelPublishServer;
	GraphicListBox listBoxPublishServer;

	GraphicLabel labelPublishServerExternalPort;
	GraphicListBox listBoxPublishServerExternalPort;
	
	GraphicMessageBox mainMessageBox;
	int mainMessageBoxState;
	
	GraphicListBox listBoxNetworkFramePeriod;
	GraphicLabel labelNetworkFramePeriod;
	
	GraphicLabel labelNetworkPauseGameForLaggedClients;
	GraphicListBox listBoxNetworkPauseGameForLaggedClients;

	GraphicLabel labelPathFinderType;
	GraphicListBox listBoxPathFinderType;
	
	GraphicLabel labelMapFilter;
	GraphicListBox listBoxMapFilter;
	
	GraphicLabel labelAdvanced;
	GraphicListBox listBoxAdvanced;

	GraphicLabel labelAllowObservers;
	GraphicListBox listBoxAllowObservers;

	GraphicLabel *activeInputLabel;

	bool needToSetChangedGameSettings;
	time_t lastSetChangedGameSettings;
	time_t lastMasterserverPublishing;
	time_t lastNetworkPing;

	bool needToRepublishToMasterserver;
	bool needToBroadcastServerSettings;
	std::map<string,string> publishToServerInfo;
	SimpleTaskThread *publishToMasterserverThread;
	Mutex masterServerThreadAccessor;
	
	bool parentMenuIsMs;
	int soundConnectionCount;
	
	bool showMasterserverError;
	string masterServererErrorToShow;

	bool showGeneralError;
	string generalErrorToShow;
	bool serverInitError;
	
	Console console;
	ChatManager chatManager;
	bool showFullConsole;

	string lastMapDataSynchError;
	string lastTileDataSynchError;
	string lastTechtreeDataSynchError;

	string defaultPlayerName;
	int8 switchSetupRequestFlagType;

	bool enableFactionTexturePreview;
	string currentFactionLogo;
	Texture2D *factionTexture;

	MapPreview mapPreview;

public:
	MenuStateCustomGame(Program *program, MainMenu *mainMenu ,bool openNetworkSlots= false, bool parentMenuIsMasterserver=false);
	~MenuStateCustomGame();

	void mouseClick(int x, int y, MouseButton mouseButton);
	void mouseMove(int x, int y, const MouseState *mouseState);
	void render();
	void update();

    virtual void keyDown(char key);
    virtual void keyPress(char c);
    virtual void keyUp(char key);
    

    virtual void simpleTask();
    virtual bool isInSpecialKeyCaptureEvent() { return chatManager.getEditEnabled(); }

private:

    bool hasNetworkGameSettings();
    void loadGameSettings(GameSettings *gameSettings);
	void loadMapInfo(string file, MapInfo *mapInfo,bool loadMapPreview);
	void reloadFactions();
	void updateControlers();
	void closeUnusedSlots();
	void updateNetworkSlots();
	void publishToMasterserver();
	void returnToParentMenu();
	void showMessageBox(const string &text, const string &header, bool toggle);

	void saveGameSettingsToFile(std::string fileName);
	void switchToNextMapGroup(const int direction);
	string getCurrentMapFile();
	GameSettings loadGameSettingsFromFile(std::string fileName);
	void setActiveInputLabel(GraphicLabel *newLable);
	string getHumanPlayerName(int index=-1);

	void cleanupFactionTexture();
	void loadFactionTexture(string filepath);

	void renderMap(	const MapPreview *map, int x, int y, int clientW,
					int clientH, int cellSize, int screenX, int screenY);
};

}}//end namespace

#endif
