// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2010-  by Titus Tscharntke
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "menu_state_masterserver.h"

#include "server_line.h"
#include "renderer.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "config.h"
#include "menu_state_connected_game.h"
#include "menu_state_custom_game.h"
#include "menu_state_root.h"
#include "metrics.h"
#include "network_manager.h"
#include "network_message.h"
#include "auto_test.h"
#include "socket.h"
#include "masterserver_info.h"
#include <curl/curl.h>

#include "leak_dumper.h"

namespace Glest{ namespace Game{

DisplayMessageFunction MenuStateMasterserver::pCB_DisplayMessage = NULL;

static const char *IRC_SERVER   = "irc.freenode.net";
static const char *IRC_CHANNEL  = "#megaglest-lobby";

// =====================================================
// 	class MenuStateMasterserver
// =====================================================

MenuStateMasterserver::MenuStateMasterserver(Program *program, MainMenu *mainMenu):
	MenuState(program, mainMenu, "masterserver")
{
	containerName = "MasterServer";
	updateFromMasterserverThread = NULL;
	ircClient = NULL;
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	Lang &lang= Lang::getInstance();

	//Configure ConsolePosition
	consoleIRC.setYPos(60);
	consoleIRC.setFont(CoreData::getInstance().getMenuFontNormal());
	consoleIRC.setLineHeight(18);

	serverLinesToRender=9;
	serverLinesLineHeight=25;
	serverLinesYBase=680;
	
	userButtonsYBase=serverLinesYBase-(serverLinesToRender+2)*serverLinesLineHeight;
	userButtonsHeight=20;
	userButtonsWidth=150;
	userButtonsLineHeight=userButtonsHeight+2;
	userButtonsToRender=userButtonsYBase/userButtonsLineHeight;
	userButtonsXBase=1000-userButtonsWidth;
	
	lines[0].init(0, userButtonsYBase+serverLinesLineHeight);
	lines[1].init(userButtonsXBase-5,0,5,userButtonsYBase+userButtonsLineHeight);
	lines[1].setHorizontal(false);
	
	autoRefreshTime=0;
	playServerFoundSound=false;
	announcementLoaded=false;

	mainMessageBox.registerGraphicComponent(containerName,"mainMessageBox");
	mainMessageBox.init(lang.get("Ok"));
	mainMessageBox.setEnabled(false);
	mainMessageBoxState=0;

	lastRefreshTimer= time(NULL);

	// announcement
	announcementLabel.registerGraphicComponent(containerName,"announcementLabel");
    announcementLabel.init(10, 730);
    announcementLabel.setFont(CoreData::getInstance().getMenuFontBig());
    announcementLabel.setText("");

    // versionInfo
    versionInfoLabel.registerGraphicComponent(containerName,"versionInfoLabel");
    versionInfoLabel.init(10, 680);
    versionInfoLabel.setFont(CoreData::getInstance().getMenuFontBig());
    versionInfoLabel.setText("");
    
	// header
	labelTitle.registerGraphicComponent(containerName,"labelTitle");
	labelTitle.init(330, serverLinesYBase+40);
	labelTitle.setFont(CoreData::getInstance().getMenuFontBig());
	labelTitle.setText(lang.get("AvailableServers"));

	if(Config::getInstance().getString("Masterserver","") == "") {
		labelTitle.setText("*** " + lang.get("AvailableServers"));
	}

	// bottom
	int buttonPos=230;

	// Titles for current games - START
	int lineIndex = 0;
	int lineOffset=25*lineIndex;
	int i=10;
	int startOffset=serverLinesYBase+23;
	
	//general info:
	i+=10;
	glestVersionLabel.registerGraphicComponent(containerName,"glestVersionLabel");
	glestVersionLabel.init(i,startOffset-lineOffset);
	glestVersionLabel.setText(lang.get("MGVersion"));

	i+=80;
	platformLabel.registerGraphicComponent(containerName,"platformLabel");
	platformLabel.init(i,startOffset-lineOffset);
	platformLabel.setText(lang.get("MGPlatform"));

//	i+=50;
//	binaryCompileDateLabel.registerGraphicComponent(containerName,"binaryCompileDateLabel");
//	binaryCompileDateLabel.init(i,startOffset-lineOffset);
//	binaryCompileDateLabel.setText(lang.get("MGBuildDateTime"));

	//game info:
	i+=130;
	serverTitleLabel.registerGraphicComponent(containerName,"serverTitleLabel");
	serverTitleLabel.init(i,startOffset-lineOffset);
	serverTitleLabel.setText(lang.get("MGGameTitle"));

	i+=210;
	ipAddressLabel.registerGraphicComponent(containerName,"ipAddressLabel");
	ipAddressLabel.init(i,startOffset-lineOffset);
	ipAddressLabel.setText(lang.get("MGGameIP"));

	//game setup info:
	i+=100;
	techLabel.registerGraphicComponent(containerName,"techLabel");
	techLabel.init(i,startOffset-lineOffset);
	techLabel.setText(lang.get("TechTree"));

	i+=100;
	mapLabel.registerGraphicComponent(containerName,"mapLabel");
	mapLabel.init(i,startOffset-lineOffset);
	mapLabel.setText(lang.get("Map"));

	i+=100;
	tilesetLabel.registerGraphicComponent(containerName,"tilesetLabel");
	tilesetLabel.init(i,startOffset-lineOffset);
	tilesetLabel.setText(lang.get("Tileset"));

	i+=100;
	activeSlotsLabel.registerGraphicComponent(containerName,"activeSlotsLabel");
	activeSlotsLabel.init(i,startOffset-lineOffset);
	activeSlotsLabel.setText(lang.get("MGGameSlots"));

	i+=50;
	externalConnectPort.registerGraphicComponent(containerName,"externalConnectPort");
	externalConnectPort.init(i,startOffset-lineOffset);
	externalConnectPort.setText(lang.get("Port"));

	i+=50;
	selectButton.registerGraphicComponent(containerName,"selectButton");
	selectButton.init(i, startOffset-lineOffset);
	selectButton.setText(lang.get("MGJoinGameSlots"));

	// Titles for current games - END

	buttonReturn.registerGraphicComponent(containerName,"buttonReturn");
    buttonReturn.init(50, buttonPos, 150);

    buttonCreateGame.registerGraphicComponent(containerName,"buttonCreateGame");
    buttonCreateGame.init(300, buttonPos, 150);

    buttonRefresh.registerGraphicComponent(containerName,"buttonRefresh");
    buttonRefresh.init(550, buttonPos, 150);

	buttonRefresh.setText(lang.get("RefreshList"));
	buttonReturn.setText(lang.get("Return"));
	buttonCreateGame.setText(lang.get("CustomGame"));
	labelAutoRefresh.setText(lang.get("AutoRefreshRate"));

	labelAutoRefresh.registerGraphicComponent(containerName,"labelAutoRefresh");
	labelAutoRefresh.init(800,buttonPos+30);

	listBoxAutoRefresh.registerGraphicComponent(containerName,"listBoxAutoRefresh");
	listBoxAutoRefresh.init(800,buttonPos);
	listBoxAutoRefresh.pushBackItem(lang.get("Off"));
	listBoxAutoRefresh.pushBackItem("10 s");
	listBoxAutoRefresh.pushBackItem("20 s");
	listBoxAutoRefresh.pushBackItem("30 s");
	listBoxAutoRefresh.setSelectedItemIndex(1);
	autoRefreshTime=10*listBoxAutoRefresh.getSelectedItemIndex();

	ircOnlinePeopleLabel.registerGraphicComponent(containerName,"ircOnlinePeopleLabel");
	ircOnlinePeopleLabel.init(userButtonsXBase,userButtonsYBase);
	ircOnlinePeopleLabel.setText("IRC People Online:");

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	NetworkManager::getInstance().end();
	NetworkManager::getInstance().init(nrClient);
	
	//console.addLine(lang.get("To switch off music press")+" - \""+configKeys.getCharKey("ToggleMusic")+"\"");

	GraphicComponent::applyAllCustomProperties(containerName);

    char szIRCNick[80]="";
    srand(time(NULL));
    int randomNickId = rand() % 999;
    string netPlayerName=Config::getInstance().getString("NetPlayerName",Socket::getHostName().c_str());
    string ircname=netPlayerName.substr(0,9);
    sprintf(szIRCNick,"MG_%s_%d",ircname.c_str(),randomNickId);

    lines[2].init(0,consoleIRC.getYPos()-10,userButtonsXBase,5);
    chatManager.init(&consoleIRC, -1, true, szIRCNick);
    chatManager.setXPos(0);
    chatManager.setYPos(consoleIRC.getYPos()-20);
    chatManager.setFont(CoreData::getInstance().getMenuFontNormal());

	MutexSafeWrapper safeMutexPtr(&masterServerThreadPtrChangeAccessor);
	masterServerThreadInDeletion = false;
	needUpdateFromServer = true;
	updateFromMasterserverThread = new SimpleTaskThread(this,0,100);
	updateFromMasterserverThread->setUniqueID(__FILE__);
	updateFromMasterserverThread->start();

    ircArgs.push_back(IRC_SERVER);
    ircArgs.push_back(szIRCNick);
    ircArgs.push_back(IRC_CHANNEL);
    ircClient = new IRCThread(ircArgs,this);
    ircClient->setUniqueID(__FILE__);
    ircClient->start();

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void MenuStateMasterserver::setConsolePos(int yPos){
		consoleIRC.setYPos(yPos);
		lines[2].setY(consoleIRC.getYPos()-10);
		chatManager.setYPos(consoleIRC.getYPos()-20);
}

void MenuStateMasterserver::setButtonLinePosition(int pos){
    buttonReturn.setY(pos);
    buttonCreateGame.setY(pos);
    buttonRefresh.setY(pos);
	labelAutoRefresh.setY(pos+30);
	listBoxAutoRefresh.setY(pos);
}

void MenuStateMasterserver::IRC_CallbackEvent(const char* origin, const char **params, unsigned int count) {
    //printf ("===> IRC: '%s' said in channel %s: %s\n",origin ? origin : "someone",params[0], params[1] );
    char szBuf[4096]="";
    sprintf(szBuf,"%s: %s",origin ? origin : "someone",params[1]);
    consoleIRC.addLine(szBuf);
}

void MenuStateMasterserver::cleanup() {
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    //printf("In [%s::%s Line: %d] [%p]\n",__FILE__,__FUNCTION__,__LINE__,ircClient);

	if(masterServerThreadInDeletion == false) {
		MutexSafeWrapper safeMutexPtr(&masterServerThreadPtrChangeAccessor);
		if(updateFromMasterserverThread != NULL) {
			MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
			needUpdateFromServer = false;
			safeMutex.ReleaseLock();

			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			//BaseThread::shutdownAndWait(updateFromMasterserverThread);
			masterServerThreadInDeletion = true;
			if(updateFromMasterserverThread != NULL &&
				updateFromMasterserverThread->shutdownAndWait() == true) {
				delete updateFromMasterserverThread;
			}
			updateFromMasterserverThread = NULL;
			masterServerThreadInDeletion = false;
			safeMutexPtr.ReleaseLock();
		}
		else {
			safeMutexPtr.ReleaseLock();
		}
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	clearServerLines();
	clearUserButtons();

    //printf("Exiting master server menu [%p]\n",ircClient);
    if(ircClient != NULL) {
        SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

        ircClient->signalQuit();
        //if(ircClient->shutdownAndWait() == true) {
            SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
            //delete ircClient;
        //}
        SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
        ircClient = NULL;
    }

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] END\n",__FILE__,__FUNCTION__,__LINE__);
}

MenuStateMasterserver::~MenuStateMasterserver() {
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    //printf("In [%s::%s Line: %d] [%p]\n",__FILE__,__FUNCTION__,__LINE__,ircClient);
	cleanup();
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] END\n",__FILE__,__FUNCTION__,__LINE__);
}

void MenuStateMasterserver::clearServerLines() {
	while(!serverLines.empty()){
		delete serverLines.back();
		serverLines.pop_back();
	}
}

void MenuStateMasterserver::clearUserButtons() {
	while(!userButtons.empty()){
		delete userButtons.back();
		userButtons.pop_back();
	}
}

void MenuStateMasterserver::mouseClick(int x, int y, MouseButton mouseButton){
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	CoreData &coreData= CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

	if(mainMessageBox.getEnabled()){
		int button= 1;
		if(mainMessageBox.mouseClick(x, y, button))
		{
			soundRenderer.playFx(coreData.getClickSoundA());
			if(button==1)
			{
				mainMessageBox.setEnabled(false);
			}
		}
	}
	else if(buttonRefresh.mouseClick(x, y)){
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
		soundRenderer.playFx(coreData.getClickSoundB());
		needUpdateFromServer = true;

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    }
    else if(buttonReturn.mouseClick(x, y)){
    	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    	MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
		soundRenderer.playFx(coreData.getClickSoundB());

		//BaseThread::shutdownAndWait(updateFromMasterserverThread);
		MutexSafeWrapper safeMutexPtr(&masterServerThreadPtrChangeAccessor);
		masterServerThreadInDeletion = true;
		if(updateFromMasterserverThread != NULL &&
			updateFromMasterserverThread->shutdownAndWait() == true) {
			delete updateFromMasterserverThread;
		}
		updateFromMasterserverThread = NULL;
		masterServerThreadInDeletion = false;
		safeMutexPtr.ReleaseLock();

		safeMutex.ReleaseLock();

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

        cleanup();

        SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		mainMenu->setState(new MenuStateRoot(program, mainMenu));

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    }
    else if(buttonCreateGame.mouseClick(x, y)){
    	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    	MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
		soundRenderer.playFx(coreData.getClickSoundB());
		needUpdateFromServer = false;
		safeMutex.ReleaseLock();

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		//BaseThread::shutdownAndWait(updateFromMasterserverThread);
		MutexSafeWrapper safeMutexPtr(&masterServerThreadPtrChangeAccessor);
		masterServerThreadInDeletion = true;
		if(updateFromMasterserverThread != NULL &&
			updateFromMasterserverThread->shutdownAndWait() == true) {
			delete updateFromMasterserverThread;
		}
		updateFromMasterserverThread = NULL;
		masterServerThreadInDeletion = false;
		safeMutexPtr.ReleaseLock();

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

        cleanup();
        SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		mainMenu->setState(new MenuStateCustomGame(program, mainMenu,true,true));

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    }
    else if(listBoxAutoRefresh.mouseClick(x, y)){
    	MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
		soundRenderer.playFx(coreData.getClickSoundA());
		autoRefreshTime=10*listBoxAutoRefresh.getSelectedItemIndex();
    }
    else {
    	MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
    	bool clicked=false;
	    for(int i=0; i<serverLines.size() && i<serverLinesToRender; ++i){
	    	if(serverLines[i]->buttonMouseClick(x, y)) {
	    		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
				clicked=true;
	    		soundRenderer.playFx(coreData.getClickSoundB());
	    		string connectServerIP = serverLines[i]->getMasterServerInfo()->getIpAddress();
	    		int connectServerPort = serverLines[i]->getMasterServerInfo()->getExternalConnectPort();
	    		connectToServer(connectServerIP,connectServerPort);
	    		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	    		safeMutex.ReleaseLock();

	    		MutexSafeWrapper safeMutexPtr(&masterServerThreadPtrChangeAccessor);
	    		masterServerThreadInDeletion = true;
				if(updateFromMasterserverThread != NULL &&
					updateFromMasterserverThread->shutdownAndWait() == true) {
					delete updateFromMasterserverThread;
				}
	    		updateFromMasterserverThread = NULL;
	    		masterServerThreadInDeletion = false;
	    		safeMutexPtr.ReleaseLock();

	    		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	    		mainMenu->setState(new MenuStateConnectedGame(program, mainMenu,jmMasterserver));
	    		break;
	    	}
	    }
	    if(!clicked){
		    for(int i = 0; i < userButtons.size(); ++i) {
	       		if(userButtons[i]->mouseClick(x, y)) {
	       			clicked=true;
	     			if(!chatManager.getEditEnabled())
	     			{
	     				chatManager.switchOnEdit();
	     				chatManager.addText(userButtons[i]->getText()+" ");
	     			}
	     			else
	     			{
	     				chatManager.addText(userButtons[i]->getText());
	     			}
	       			break;
	       		}
	    	}
	    }
    }
}

void MenuStateMasterserver::mouseMove(int x, int y, const MouseState *ms){
	MutexSafeWrapper safeMutex(&masterServerThreadAccessor);

	if (mainMessageBox.getEnabled()) {
		mainMessageBox.mouseMove(x, y);
	}
    buttonRefresh.mouseMove(x, y);
    buttonReturn.mouseMove(x, y);
    buttonCreateGame.mouseMove(x, y);
    listBoxAutoRefresh.mouseMove(x, y);

    for(int i=0; i<serverLines.size() && i<serverLinesToRender; ++i){
    	serverLines[i]->buttonMouseMove(x, y);
    }
    for(int i = 0; i < userButtons.size(); ++i) {
       	userButtons[i]->mouseMove(x, y);
    }
    
}

void MenuStateMasterserver::render(){
	Renderer &renderer= Renderer::getInstance();

	MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
	if(mainMessageBox.getEnabled()) {
		renderer.renderMessageBox(&mainMessageBox);
	}
	else
	{
		renderer.renderLabel(&labelTitle,&GREEN);
		renderer.renderLabel(&announcementLabel,&YELLOW);
		renderer.renderLabel(&versionInfoLabel);

		// Render titles for server games listed
		const Vec4f titleLabelColor = CYAN;

		//general info:
		renderer.renderLabel(&glestVersionLabel,&titleLabelColor);
		renderer.renderLabel(&platformLabel,&titleLabelColor);
		//renderer.renderLabel(&binaryCompileDateLabel,&titleLabelColor);

		//game info:
		renderer.renderLabel(&serverTitleLabel,&titleLabelColor);
		renderer.renderLabel(&ipAddressLabel,&titleLabelColor);

		//game setup info:
		renderer.renderLabel(&techLabel,&titleLabelColor);
		renderer.renderLabel(&mapLabel,&titleLabelColor);
		renderer.renderLabel(&tilesetLabel,&titleLabelColor);
		renderer.renderLabel(&activeSlotsLabel,&titleLabelColor);
		renderer.renderLabel(&externalConnectPort,&titleLabelColor);
		renderer.renderLabel(&selectButton,&titleLabelColor);

        if(ircClient != NULL &&
           ircClient->isConnected() == true &&
           ircClient->getHasJoinedChannel() == true) {
            const Vec4f titleLabelColor = GREEN;
            renderer.renderLabel(&ircOnlinePeopleLabel,&titleLabelColor);
        }
        else {
            const Vec4f titleLabelColor = RED;
            renderer.renderLabel(&ircOnlinePeopleLabel,&titleLabelColor);
        }
        const Vec4f titleLabelColorList = YELLOW;

		for(int i=0; i<serverLines.size() && i<serverLinesToRender; ++i){
	    	serverLines[i]->render();
	    }
	    for(int i=0; i<sizeof(lines) / sizeof(lines[0]); ++i){
	    	renderer.renderLine(&lines[i]);
	    }
	    renderer.renderButton(&buttonRefresh);
		renderer.renderButton(&buttonReturn);
		renderer.renderLabel(&labelAutoRefresh);
		renderer.renderButton(&buttonCreateGame);
		renderer.renderListBox(&listBoxAutoRefresh);
	    
		for(int i = 0; i < userButtons.size(); ++i) {
        	renderer.renderButton(userButtons[i]);
        }
        
   		renderer.renderChatManager(&chatManager);
		renderer.renderConsole(&consoleIRC,true,true);
		
	}
	if(program != NULL) program->renderProgramMsgBox();
}

void MenuStateMasterserver::update() {
	MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
	if(autoRefreshTime!=0 && difftime(time(NULL),lastRefreshTimer) >= autoRefreshTime ) {
		needUpdateFromServer = true;
		lastRefreshTimer= time(NULL);
	}

	// calculate button linepos:
	
	setButtonLinePosition(serverLinesYBase-serverLinesToRender*serverLinesLineHeight);

	if(playServerFoundSound)
	{
		SoundRenderer::getInstance().playFx(CoreData::getInstance().getAttentionSound());
		//switch on music again!!
		Config &config = Config::getInstance();
		float configVolume = (config.getInt("SoundVolumeMusic") / 100.f);
		CoreData::getInstance().getMenuMusic()->setVolume(configVolume);

		playServerFoundSound=false;
	}

	//console.update();

    //call the chat manager
    chatManager.updateNetwork();

    //console
    consoleIRC.update();

	
    if(ircClient != NULL) {
        std::vector<string> nickList = ircClient->getNickList();
        bool isNew=false;
        //check if there is something new
        if( oldNickList.size()!=nickList.size()) {
        	isNew=true;
        }
        else {
        	for(int i = 0; i < nickList.size(); ++i) {
        		if(nickList[i]!=oldNickList[i])
        		{
        			isNew=true;
        			break;
        		}
        	}
        }
        if(isNew) {
	        clearUserButtons();
	        for(int i = 0; i < nickList.size(); ++i) {
	                GraphicButton *button=new GraphicButton();
	                button->init(userButtonsXBase,userButtonsYBase-userButtonsLineHeight*(i+1),userButtonsWidth,userButtonsHeight);
					button->setFont(CoreData::getInstance().getDisplayFontSmall());
					button->setText(nickList[i]);
	                userButtons.push_back(button);
	        }
	        oldNickList=nickList;
        }
    }

	if(threadedErrorMsg != "") {
		std::string sError = threadedErrorMsg;
		threadedErrorMsg = "";

		if(pCB_DisplayMessage != NULL) {
			pCB_DisplayMessage(sError.c_str(),false);
		}
		else {
			throw runtime_error(sError.c_str());
		}
	}
}

void MenuStateMasterserver::simpleTask() {
	if(masterServerThreadInDeletion == true) {
		return;
	}

	if( updateFromMasterserverThread == NULL ||
		updateFromMasterserverThread->getQuitStatus() == true) {
		return;
	}
	MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
	bool needUpdate = needUpdateFromServer;
	safeMutex.ReleaseLock();

	if(needUpdate == true) {
		updateServerInfo();
	}
}

void MenuStateMasterserver::updateServerInfo() {
	try {

		if(masterServerThreadInDeletion == true) {
			return;
		}

		MutexSafeWrapper safeMutexPtr(&masterServerThreadPtrChangeAccessor);
		if( updateFromMasterserverThread == NULL ||
			updateFromMasterserverThread->getQuitStatus() == true) {
			safeMutexPtr.ReleaseLock();
			return;
		}
		safeMutexPtr.ReleaseLock(true);

		MutexSafeWrapper safeMutex(&masterServerThreadAccessor);
		needUpdateFromServer = false;
		int numberOfOldServerLines=serverLines.size();
		clearServerLines();
		safeMutex.ReleaseLock(true);

		if(announcementLoaded == false) {
			string announcementURL = Config::getInstance().getString("AnnouncementURL","http://master.megaglest.org/files/announcement.txt");
			if(announcementURL != "") {
				std::string announcementTxt = SystemFlags::getHTTP(announcementURL);
				if(StartsWith(announcementTxt,"Announcement from Masterserver:") == true) {
					int newlineCount=0;
					size_t lastIndex=0;
					
					//announcementLabel.setText(announcementTxt);
					consoleIRC.addLine(announcementTxt);
					
					while(true){
						lastIndex=announcementTxt.find("\n",lastIndex+1);
						if(lastIndex==string::npos)
						{
							break;
						}
						else
						{
							newlineCount++;
						}
					}
					newlineCount--;// remove my own line
					for( int i=0; i< newlineCount;++i ){
						consoleIRC.addLine("");
					}
				}
			}
			consoleIRC.addLine("---------------------------------------------");
			string versionURL = Config::getInstance().getString("VersionURL","http://master.megaglest.org/files/versions/")+glestVersionString+".txt";
			//printf("\nversionURL=%s\n",versionURL.c_str());
			if(versionURL != "") {
				std::string versionTxt = SystemFlags::getHTTP(versionURL);
				if(StartsWith(versionTxt,"Version info:") == true) {
					int newlineCount=0;
					size_t lastIndex=0;
					
					//versionInfoLabel.setText(versionTxt);
					consoleIRC.addLine(versionTxt);
					
					while(true){
						lastIndex=versionTxt.find("\n",lastIndex+1);
						if(lastIndex==string::npos)
						{
							break;
						}
						else
						{
							newlineCount++;
						}
					}
					newlineCount--;// remove my own line
					for( int i=0; i< newlineCount;++i ){
						consoleIRC.addLine("");
					}
				}
			}
			consoleIRC.addLine("---------------------------------------------");
			// write hint to console:
			Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));
			consoleIRC.addLine(Lang::getInstance().get("To switch off music press")+" - \""+configKeys.getCharKey("ToggleMusic")+"\"");
			
			announcementLoaded=true;
		}

        Lang &lang= Lang::getInstance();
        try {
            if(Config::getInstance().getString("Masterserver","") != "") {
                std::string serverInfo = SystemFlags::getHTTP(Config::getInstance().getString("Masterserver") + "showServersForGlest.php");

                if(serverInfo != "") {
                    std::vector<std::string> serverList;
                    Tokenize(serverInfo,serverList,"\n");
                    for(int i=0; i < serverList.size(); i++) {
                        string &server = serverList[i];
                        std::vector<std::string> serverEntities;
                        Tokenize(server,serverEntities,"|");

                        const int MIN_FIELDS_EXPECTED = 12;
                        if(serverEntities.size() >= MIN_FIELDS_EXPECTED) {
                            labelTitle.setText(lang.get("AvailableServers"));

                            if(Config::getInstance().getString("Masterserver","") == "") {
                                labelTitle.setText("*** " + lang.get("AvailableServers"));
                            }

                            MasterServerInfo *masterServerInfo=new MasterServerInfo();

                            //general info:
                            masterServerInfo->setGlestVersion(serverEntities[0]);
                            masterServerInfo->setPlatform(serverEntities[1]);
                            masterServerInfo->setBinaryCompileDate(serverEntities[2]);

                            //game info:
                            masterServerInfo->setServerTitle(serverEntities[3]);
                            masterServerInfo->setIpAddress(serverEntities[4]);

                            //game setup info:
                            masterServerInfo->setTech(serverEntities[5]);
                            masterServerInfo->setMap(serverEntities[6]);
                            masterServerInfo->setTileset(serverEntities[7]);
                            masterServerInfo->setActiveSlots(strToInt(serverEntities[8]));
                            masterServerInfo->setNetworkSlots(strToInt(serverEntities[9]));
                            masterServerInfo->setConnectedClients(strToInt(serverEntities[10]));
                            masterServerInfo->setExternalConnectPort(strToInt(serverEntities[11]));

                            //printf("Getting Ping time for host %s\n",masterServerInfo->getIpAddress().c_str());
                            //float pingTime = Socket::getAveragePingMS(masterServerInfo->getIpAddress().c_str(),1);
                            //printf("Ping time = %f\n",pingTime);
                            char szBuf[1024]="";
                            //sprintf(szBuf,"%s, %.2fms",masterServerInfo->getServerTitle().c_str(),pingTime);
                            sprintf(szBuf,"%s",masterServerInfo->getServerTitle().c_str());
                            masterServerInfo->setServerTitle(szBuf);

                            safeMutexPtr.Lock();
                            if( masterServerThreadInDeletion == true ||
                                updateFromMasterserverThread == NULL ||
                                updateFromMasterserverThread->getQuitStatus() == true) {
                                safeMutexPtr.ReleaseLock();
                                return;
                            }
                            safeMutexPtr.ReleaseLock(true);

                            safeMutex.Lock();
                            serverLines.push_back(new ServerLine( masterServerInfo, i, serverLinesYBase, serverLinesLineHeight, containerName));
                            safeMutex.ReleaseLock(true);
                        }
                        else {
                            Lang &lang= Lang::getInstance();
                            labelTitle.setText("*** " + lang.get("AvailableServers") + " [" + serverInfo + "]");

                            SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line %d] error, no masterserver defined!\n",__FILE__,__FUNCTION__,__LINE__);
                        }
                    }
                }
            }
        }
		catch(const exception &ex) {
		    labelTitle.setText("*** " + lang.get("AvailableServers") + " [" + ex.what() + "]");
		    SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line %d] error during Internet game status update: [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		}

		safeMutexPtr.Lock();
		if( updateFromMasterserverThread == NULL ||
			updateFromMasterserverThread->getQuitStatus() == true) {
			safeMutexPtr.ReleaseLock();
			return;
		}
		safeMutexPtr.ReleaseLock();

		safeMutex.Lock();
		if(serverLines.size()>numberOfOldServerLines) {
			playServerFoundSound=true;
		}
		safeMutex.ReleaseLock();
	}
	catch(const exception &e){
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] Line: %d, error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		threadedErrorMsg = e.what();
	}
}


bool MenuStateMasterserver::connectToServer(string ipString, int port)
{
    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] START  ipString='%s'\n",__FILE__,__FUNCTION__,ipString.c_str());

	ClientInterface* clientInterface= NetworkManager::getInstance().getClientInterface();
	Config& config= Config::getInstance();
	Ip serverIp(ipString);

	//int serverPort = Config::getInstance().getInt("ServerPort",intToStr(GameConstants::serverPort).c_str());
	int serverPort = port;
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] try to connect to [%s] serverPort = %d\n",__FILE__,__FUNCTION__,serverIp.getString().c_str(),serverPort);
	clientInterface->connect(serverIp, serverPort);
	if(clientInterface->isConnected() == false) {
		NetworkManager::getInstance().end();
		NetworkManager::getInstance().init(nrClient);

		mainMessageBoxState=1;
		Lang &lang= Lang::getInstance();
		showMessageBox(lang.get("Couldnt connect"), lang.get("Connection failed"), false);
		return false;

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] connection failed\n",__FILE__,__FUNCTION__);
	}
	else {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] connected to [%s]\n",__FILE__,__FUNCTION__,serverIp.getString().c_str());

		//save server ip
		//config.setString("ServerIp", serverIp.getString());
		//config.save();

		return true;
	}
}

void MenuStateMasterserver::showMessageBox(const string &text, const string &header, bool toggle){
	if(!toggle){
		mainMessageBox.setEnabled(false);
	}

	if(!mainMessageBox.getEnabled()){
		mainMessageBox.setText(text);
		mainMessageBox.setHeader(header);
		mainMessageBox.setEnabled(true);
	}
	else{
		mainMessageBox.setEnabled(false);
	}
}


void MenuStateMasterserver::keyDown(char key) {
	Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));

    if(chatManager.getEditEnabled() == true) {
        //printf("keyDown key [%d] chatManager.getText() [%s]\n",key,chatManager.getText().c_str());
        if(key == vkReturn && ircClient != NULL) {
            ircClient->SendIRCCmdMessage(IRC_CHANNEL, chatManager.getText());
        }
    }

    chatManager.keyDown(key);
    if(chatManager.getEditEnabled() == false) {
        if(key == configKeys.getCharKey("ToggleMusic")) {
            Config &config = Config::getInstance();
            Lang &lang= Lang::getInstance();

            float configVolume = (config.getInt("SoundVolumeMusic") / 100.f);
            float currentVolume = CoreData::getInstance().getMenuMusic()->getVolume();
            if(currentVolume > 0) {
                CoreData::getInstance().getMenuMusic()->setVolume(0.f);
                consoleIRC.addLine(lang.get("GameMusic") + " " + lang.get("Off"));
            }
            else {
                CoreData::getInstance().getMenuMusic()->setVolume(configVolume);
                //If the config says zero, use the default music volume
                //gameMusic->setVolume(configVolume ? configVolume : 0.9);
                consoleIRC.addLine(lang.get("GameMusic"));
            }
        }
        else if(key == configKeys.getCharKey("SaveGUILayout")) {
            bool saved = GraphicComponent::saveAllCustomProperties(containerName);
            Lang &lang= Lang::getInstance();
            consoleIRC.addLine(lang.get("GUILayoutSaved") + " [" + (saved ? lang.get("Yes") : lang.get("No"))+ "]");
        }
    }
}

void MenuStateMasterserver::keyPress(char c) {
    chatManager.keyPress(c);
}
void MenuStateMasterserver::keyUp(char key) {
    chatManager.keyUp(key);

    if(chatManager.getEditEnabled()) {
        //send key to the chat manager
        chatManager.keyUp(key);
    }
}

}}//end namespace
