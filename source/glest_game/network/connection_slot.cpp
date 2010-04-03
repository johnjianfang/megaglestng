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

#include "connection_slot.h"

#include <stdexcept>

#include "conversion.h"
#include "game_util.h"
#include "config.h"
#include "server_interface.h"
#include "network_message.h"
#include "leak_dumper.h"

#include "platform_util.h"
#include "map.h"

using namespace std;
using namespace Shared::Util;
//using namespace Shared::Platform;

namespace Glest{ namespace Game{

// =====================================================
//	class ClientConnection
// =====================================================

ConnectionSlot::ConnectionSlot(ServerInterface* serverInterface, int playerIndex)
{
	this->serverInterface= serverInterface;
	this->playerIndex= playerIndex;
	socket= NULL;
	ready= false;

	networkGameDataSynchCheckOkMap      = false;
	networkGameDataSynchCheckOkTile     = false;
	networkGameDataSynchCheckOkTech     = false;
	networkGameDataSynchCheckOkFogOfWar = false;

    chatText.clear();
    chatSender.clear();
    chatTeamIndex= -1;
}

ConnectionSlot::~ConnectionSlot()
{
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] START\n",__FILE__,__FUNCTION__);

	close();

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] END\n",__FILE__,__FUNCTION__);
}

void ConnectionSlot::update()
{
    update(true);
}

void ConnectionSlot::update(bool checkForNewClients)
{
	if(socket == NULL)
	{
        if(networkGameDataSynchCheckOkMap) networkGameDataSynchCheckOkMap  = false;
        if(networkGameDataSynchCheckOkTile) networkGameDataSynchCheckOkTile = false;
        if(networkGameDataSynchCheckOkTech) networkGameDataSynchCheckOkTech = false;
        if(networkGameDataSynchCheckOkFogOfWar) networkGameDataSynchCheckOkFogOfWar = false;

        // Is the listener socket ready to be read?
	    //if(serverInterface->getServerSocket()->isReadable() == true)
	    if(checkForNewClients == true)
	    {
            socket = serverInterface->getServerSocket()->accept();

            //send intro message when connected
            if(socket != NULL)
            {
                SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] accepted new client connection\n",__FILE__,__FUNCTION__);

                chatText.clear();
                chatSender.clear();
                chatTeamIndex= -1;

                NetworkMessageIntro networkMessageIntro(getNetworkVersionString(), socket->getHostName(), playerIndex);
                sendMessage(&networkMessageIntro);
            }
	    }
	}
	else
	{
		if(socket->isConnected())
		{
            chatText.clear();
            chatSender.clear();
            chatTeamIndex= -1;

			NetworkMessageType networkMessageType= getNextMessageType();

			//process incoming commands
			switch(networkMessageType){

				case nmtInvalid:
					break;

                case nmtText:
                {
                    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got nmtText\n",__FILE__,__FUNCTION__);

                    NetworkMessageText networkMessageText;
                    if(receiveMessage(&networkMessageText))
                    {
                        chatText      = networkMessageText.getText();
                        chatSender    = networkMessageText.getSender();
                        chatTeamIndex = networkMessageText.getTeamIndex();

                        SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] chatText [%s] chatSender [%s] chatTeamIndex = %d\n",__FILE__,__FUNCTION__,chatText.c_str(),chatSender.c_str(),chatTeamIndex);
                    }
                }
                break;

				//command list
				case nmtCommandList: {

				    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got nmtCommandList\n",__FILE__,__FUNCTION__);

					NetworkMessageCommandList networkMessageCommandList;
					if(receiveMessage(&networkMessageCommandList))
					{
						for(int i= 0; i<networkMessageCommandList.getCommandCount(); ++i)
						{
							serverInterface->requestCommand(networkMessageCommandList.getCommand(i));
						}
					}
				}
				break;

				//process intro messages
				case nmtIntro:
				{

				    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got nmtIntro\n",__FILE__,__FUNCTION__);

					NetworkMessageIntro networkMessageIntro;
					if(receiveMessage(&networkMessageIntro))
					{
						name= networkMessageIntro.getName();

						SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got name [%s]\n",__FILE__,__FUNCTION__,name.c_str());

                        if(getAllowGameDataSynchCheck() == true && serverInterface->getGameSettings() != NULL)
                        {
                            SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] sending NetworkMessageSynchNetworkGameData\n",__FILE__,__FUNCTION__);

                            NetworkMessageSynchNetworkGameData networkMessageSynchNetworkGameData(serverInterface->getGameSettings());
                            sendMessage(&networkMessageSynchNetworkGameData);
                        }
					}
				}
				break;

                //process datasynch messages
				case nmtSynchNetworkGameDataStatus:
				{

				    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got nmtSynchNetworkGameDataStatus\n",__FILE__,__FUNCTION__);

					NetworkMessageSynchNetworkGameDataStatus networkMessageSynchNetworkGameDataStatus;
					if(receiveMessage(&networkMessageSynchNetworkGameDataStatus))
					{
                        receivedNetworkGameStatus = true;

                        Config &config = Config::getInstance();
                        string scenarioDir = "";
                        if(serverInterface->getGameSettings()->getScenarioDir() != "") {
                            scenarioDir = serverInterface->getGameSettings()->getScenarioDir();
                            if(EndsWith(scenarioDir, ".xml") == true) {
                                scenarioDir = scenarioDir.erase(scenarioDir.size() - 4, 4);
                                scenarioDir = scenarioDir.erase(scenarioDir.size() - serverInterface->getGameSettings()->getScenario().size(), serverInterface->getGameSettings()->getScenario().size() + 1);
                            }

                            SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] gameSettings.getScenarioDir() = [%s] gameSettings.getScenario() = [%s] scenarioDir = [%s]\n",__FILE__,__FUNCTION__,__LINE__,serverInterface->getGameSettings()->getScenarioDir().c_str(),serverInterface->getGameSettings()->getScenario().c_str(),scenarioDir.c_str());
                        }

                    	//tileset
                        //world.loadTileset(config.getPathListForType(ptTilesets,scenarioDir), tilesetName, &checksum);

						//int32 tilesetCRC = getFolderTreeContentsCheckSumRecursively(string(GameConstants::folder_path_tilesets) + "/" + serverInterface->getGameSettings()->getTileset() + "/*", ".xml", NULL);
                        int32 tilesetCRC = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTilesets,scenarioDir), string("/") + serverInterface->getGameSettings()->getTileset() + string("/*"), ".xml", NULL);
						//int32 techCRC    = getFolderTreeContentsCheckSumRecursively(string(GameConstants::folder_path_techs) + "/" + serverInterface->getGameSettings()->getTech() + "/*", ".xml", NULL);
                        int32 techCRC    = getFolderTreeContentsCheckSumRecursively(config.getPathListForType(ptTechs,scenarioDir), "/" + serverInterface->getGameSettings()->getTech() + "/*", ".xml", NULL);
                        Checksum checksum;
                        string file = Map::getMapPath(serverInterface->getGameSettings()->getMap(),scenarioDir);
                        checksum.addFile(file);
                        int32 mapCRC = checksum.getSum();

                        networkGameDataSynchCheckOkMap      = (networkMessageSynchNetworkGameDataStatus.getMapCRC() == mapCRC);
                        networkGameDataSynchCheckOkTile     = (networkMessageSynchNetworkGameDataStatus.getTilesetCRC() == tilesetCRC);
                        networkGameDataSynchCheckOkTech     = (networkMessageSynchNetworkGameDataStatus.getTechCRC()    == techCRC);

                        // For testing
                        //techCRC++;

						if( networkGameDataSynchCheckOkMap      == true &&
                            networkGameDataSynchCheckOkTile     == true &&
                            networkGameDataSynchCheckOkTech     == true &&
                            networkGameDataSynchCheckOkFogOfWar == true)
                        {
                            SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] client data synch ok\n",__FILE__,__FUNCTION__);
                        }
                        else
                        {
                            SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] mapCRC = %d, remote = %d\n",__FILE__,__FUNCTION__,mapCRC,networkMessageSynchNetworkGameDataStatus.getMapCRC());
                            SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] tilesetCRC = %d, remote = %d\n",__FILE__,__FUNCTION__,tilesetCRC,networkMessageSynchNetworkGameDataStatus.getTilesetCRC());
                            SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] techCRC = %d, remote = %d\n",__FILE__,__FUNCTION__,techCRC,networkMessageSynchNetworkGameDataStatus.getTechCRC());

                            if(allowDownloadDataSynch == true)
                            {
                                // Now get all filenames with their CRC values and send to the client
                                vctFileList.clear();

                                Config &config = Config::getInstance();
                                string scenarioDir = "";
                                if(serverInterface->getGameSettings()->getScenarioDir() != "") {
                                    scenarioDir = serverInterface->getGameSettings()->getScenarioDir();
                                    if(EndsWith(scenarioDir, ".xml") == true) {
                                        scenarioDir = scenarioDir.erase(scenarioDir.size() - 4, 4);
                                        scenarioDir = scenarioDir.erase(scenarioDir.size() - serverInterface->getGameSettings()->getScenario().size(), serverInterface->getGameSettings()->getScenario().size() + 1);
                                    }

                                    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] gameSettings.getScenarioDir() = [%s] gameSettings.getScenario() = [%s] scenarioDir = [%s]\n",__FILE__,__FUNCTION__,__LINE__,serverInterface->getGameSettings()->getScenarioDir().c_str(),serverInterface->getGameSettings()->getScenario().c_str(),scenarioDir.c_str());
                                }

                                if(networkGameDataSynchCheckOkTile == false)
                                {
                                    if(tilesetCRC == 0)
                                    {
                                        //vctFileList = getFolderTreeContentsCheckSumListRecursively(string(GameConstants::folder_path_tilesets) + "/" + serverInterface->getGameSettings()->getTileset() + "/*", "", &vctFileList);
                                    	vctFileList = getFolderTreeContentsCheckSumListRecursively(config.getPathListForType(ptTilesets,scenarioDir), string("/") + serverInterface->getGameSettings()->getTileset() + string("/*"), "", &vctFileList);
                                    }
                                    else
                                    {
                                        //vctFileList = getFolderTreeContentsCheckSumListRecursively(string(GameConstants::folder_path_tilesets) + "/" + serverInterface->getGameSettings()->getTileset() + "/*", ".xml", &vctFileList);
                                    	vctFileList = getFolderTreeContentsCheckSumListRecursively(config.getPathListForType(ptTilesets,scenarioDir), "/" + serverInterface->getGameSettings()->getTileset() + "/*", ".xml", &vctFileList);
                                    }
                                }
                                if(networkGameDataSynchCheckOkTech == false)
                                {
                                    if(techCRC == 0)
                                    {
                                        //vctFileList = getFolderTreeContentsCheckSumListRecursively(string(GameConstants::folder_path_techs) + "/" + serverInterface->getGameSettings()->getTech() + "/*", "", &vctFileList);
                                    	vctFileList = getFolderTreeContentsCheckSumListRecursively(config.getPathListForType(ptTechs,scenarioDir),"/" + serverInterface->getGameSettings()->getTech() + "/*", "", &vctFileList);
                                    }
                                    else
                                    {
                                        //vctFileList = getFolderTreeContentsCheckSumListRecursively(string(GameConstants::folder_path_techs) + "/" + serverInterface->getGameSettings()->getTech() + "/*", ".xml", &vctFileList);
                                    	vctFileList = getFolderTreeContentsCheckSumListRecursively(config.getPathListForType(ptTechs,scenarioDir),"/" + serverInterface->getGameSettings()->getTech() + "/*", ".xml", &vctFileList);
                                    }
                                }
                                if(networkGameDataSynchCheckOkMap == false)
                                {
                                    vctFileList.push_back(std::pair<string,int32>(Map::getMapPath(serverInterface->getGameSettings()->getMap(),scenarioDir),mapCRC));
                                }

                                //for(int i = 0; i < vctFileList.size(); i++)
                                //{
                                NetworkMessageSynchNetworkGameDataFileCRCCheck networkMessageSynchNetworkGameDataFileCRCCheck(vctFileList.size(), 1, vctFileList[0].second, vctFileList[0].first);
                                sendMessage(&networkMessageSynchNetworkGameDataFileCRCCheck);
                                //}
                            }
                        }
					}
				}
				break;

				case nmtSynchNetworkGameDataFileCRCCheck:
				{

				    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got nmtSynchNetworkGameDataFileCRCCheck\n",__FILE__,__FUNCTION__);

					NetworkMessageSynchNetworkGameDataFileCRCCheck networkMessageSynchNetworkGameDataFileCRCCheck;
					if(receiveMessage(&networkMessageSynchNetworkGameDataFileCRCCheck))
					{
					    int fileIndex = networkMessageSynchNetworkGameDataFileCRCCheck.getFileIndex();
                        NetworkMessageSynchNetworkGameDataFileCRCCheck networkMessageSynchNetworkGameDataFileCRCCheck(vctFileList.size(), fileIndex, vctFileList[fileIndex-1].second, vctFileList[fileIndex-1].first);
                        sendMessage(&networkMessageSynchNetworkGameDataFileCRCCheck);
					}
				}
				break;

                case nmtSynchNetworkGameDataFileGet:
				{

				    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] got nmtSynchNetworkGameDataFileGet\n",__FILE__,__FUNCTION__);

					NetworkMessageSynchNetworkGameDataFileGet networkMessageSynchNetworkGameDataFileGet;
					if(receiveMessage(&networkMessageSynchNetworkGameDataFileGet))
					{
                        FileTransferInfo fileInfo;
                        fileInfo.hostType   = eServer;
                        //fileInfo.serverIP   = this->ip.getString();
                        fileInfo.serverPort = GameConstants::serverPort;
                        fileInfo.fileName   = networkMessageSynchNetworkGameDataFileGet.getFileName();

                        FileTransferSocketThread *fileXferThread = new FileTransferSocketThread(fileInfo);
                        fileXferThread->start();
					}
				}
				break;

				default:
                    {
					//throw runtime_error("Unexpected message in connection slot: " + intToStr(networkMessageType));
					string sErr = "Unexpected message in connection slot: " + intToStr(networkMessageType);
                    //sendTextMessage(sErr,-1);
                    DisplayErrorMessage(sErr);
                    return;
                    }
			}
		}
		else
		{
		    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] calling close...\n",__FILE__,__FUNCTION__);

			close();
		}
	}
}

void ConnectionSlot::close()
{
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] START\n",__FILE__,__FUNCTION__);

	delete socket;
	socket= NULL;

    chatText.clear();
    chatSender.clear();
    chatTeamIndex= -1;

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] END\n",__FILE__,__FUNCTION__);
}

bool ConnectionSlot::getFogOfWar()
{
    return networkGameDataSynchCheckOkFogOfWar;
}

bool ConnectionSlot::hasValidSocketId()
{
    bool result = (socket != NULL && socket->getSocketId() > 0);
    return result;
}

}}//end namespace
