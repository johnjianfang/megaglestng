// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martio Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_CLIENTINTERFACE_H_
#define _GLEST_GAME_CLIENTINTERFACE_H_

#include <vector>

#include "network_interface.h"
//#include "game_settings.h"

#include "socket.h"

using Shared::Platform::Ip;
using Shared::Platform::ClientSocket;
using std::vector;

namespace Glest{ namespace Game{

// =====================================================
//	class ClientInterface
// =====================================================

class ClientInterface: public GameNetworkInterface{
private:
	static const int messageWaitTimeout;
	static const int waitSleepTime;

private:
	ClientSocket *clientSocket;
	//GameSettings gameSettings;
	string serverName;
	bool introDone;
	bool launchGame;
	int playerIndex;
	bool gameSettingsReceived;
	time_t connectedTime;
	bool gotIntro;

	Ip ip;
	int port;

public:
	ClientInterface();
	virtual ~ClientInterface();

	virtual Socket* getSocket()					{return clientSocket;}
	virtual const Socket* getSocket() const		{return clientSocket;}
	virtual void close();

	//message processing
	virtual void update();
	virtual void updateLobby();
	virtual void updateKeyframe(int frameCount);
	virtual void waitUntilReady(Checksum* checksum);

	// message sending
	virtual void sendTextMessage(const string &text, int teamIndex);
	virtual void quitGame(bool userManuallyQuit);

	//misc
	virtual string getNetworkStatus() ;

	//accessors
	string getServerName() const			{return serverName;}
	bool getLaunchGame() const				{return launchGame;}
	bool getIntroDone() const				{return introDone;}
	bool getGameSettingsReceived() const	{return gameSettingsReceived;}
	void setGameSettingsReceived(bool value)	{gameSettingsReceived=value;}
	int getPlayerIndex() const				{return playerIndex;}
	//const GameSettings *getGameSettings()	{return &gameSettings;}

	void connect(const Ip &ip, int port);
	void reset();

	void discoverServers(DiscoveredServersInterface *cb);
	void stopServerDiscovery();
	
	void sendSwitchSetupRequest(string selectedFactionName, int8 currentFactionIndex, int8 toFactionIndex, int8 toTeam);
	virtual bool getConnectHasHandshaked() const { return gotIntro; }
	std::string getServerIpAddress();

protected:

	Mutex * getServerSynchAccessor() { return NULL; }

private:

	void waitForMessage();
};

}}//end namespace

#endif
